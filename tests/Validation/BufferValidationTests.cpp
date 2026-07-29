#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/BufferValidation.h>

namespace
{
	spall::BufferCreateInfo bufferCreateInfo(
		spall::BufferUsageFlags usage,
		spall::ResourceStateFlags initialState)
	{
		spall::BufferCreateInfo info = {};
		info.Size = 256;
		info.Usage = usage;
		info.InitialState = initialState;

		return info;
	}
} // namespace

TEST_CASE(
	"A buffer rejects a zero size",
	"[buffer]")
{
	spall::BufferCreateInfo info = bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::Common);
	info.Size = 0;

	CHECK(spall::validateBufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer requires at least one usage flag",
	"[buffer]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(spall::BufferUsageFlags::None, spall::ResourceStateFlags::Common);

	CHECK(spall::validateBufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer rejects unknown usage flags",
	"[buffer]")
{
	const spall::BufferUsageFlags unknown = static_cast<spall::BufferUsageFlags>(1u << 20);
	const spall::BufferCreateInfo info = bufferCreateInfo(spall::BufferUsageFlags::Vertex | unknown, spall::ResourceStateFlags::Common);

	CHECK(spall::validateBufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer accepts the default Common initial state",
	"[buffer][state]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::Common);

	CHECK(info.InitialState == spall::ResourceStateFlags::Common);
	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A buffer rejects an unknown initial state",
	"[buffer][state]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::Unknown);

	CHECK(spall::validateBufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Common cannot combine with another buffer state",
	"[buffer][state]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(
		spall::BufferUsageFlags::Vertex,
		spall::ResourceStateFlags::Common | spall::ResourceStateFlags::VertexBuffer);

	CHECK(spall::validateBufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Each buffer state requires its matching usage flag",
	"[buffer][state]")
{
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Index, spall::ResourceStateFlags::VertexBuffer)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::IndexBuffer)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::ConstantBuffer)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::ShaderResource)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::UnorderedAccess)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::CopySource)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::CopyDest)) != spall::SUCCESS);
}

TEST_CASE(
	"Each buffer state is accepted alongside its usage flag",
	"[buffer][state]")
{
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Vertex, spall::ResourceStateFlags::VertexBuffer)) == spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Index, spall::ResourceStateFlags::IndexBuffer)) == spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Uniform, spall::ResourceStateFlags::ConstantBuffer)) == spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Storage, spall::ResourceStateFlags::ShaderResource)) == spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::Storage, spall::ResourceStateFlags::UnorderedAccess)) == spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::TransferSource, spall::ResourceStateFlags::CopySource)) == spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(spall::BufferUsageFlags::TransferDestination, spall::ResourceStateFlags::CopyDest)) == spall::SUCCESS);
}

TEST_CASE(
	"A buffer combines states when every matching usage is present",
	"[buffer][state]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(
		spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index | spall::BufferUsageFlags::TransferDestination,
		spall::ResourceStateFlags::VertexBuffer | spall::ResourceStateFlags::IndexBuffer | spall::ResourceStateFlags::CopyDest);

	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A combined buffer state still requires every matching usage",
	"[buffer][state]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(
		spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index,
		spall::ResourceStateFlags::VertexBuffer | spall::ResourceStateFlags::IndexBuffer | spall::ResourceStateFlags::CopyDest);

	CHECK(spall::validateBufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer rejects texture-only states",
	"[buffer][state]")
{
	const spall::BufferUsageFlags everyUsage =
		spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index | spall::BufferUsageFlags::Uniform |
		spall::BufferUsageFlags::TransferSource | spall::BufferUsageFlags::TransferDestination | spall::BufferUsageFlags::Storage;

	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(everyUsage, spall::ResourceStateFlags::RenderTarget)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(everyUsage, spall::ResourceStateFlags::DepthWrite)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(everyUsage, spall::ResourceStateFlags::DepthRead)) != spall::SUCCESS);
	CHECK(spall::validateBufferCreateInfo(bufferCreateInfo(everyUsage, spall::ResourceStateFlags::Present)) != spall::SUCCESS);
}

TEST_CASE(
	"Storage usage serves both shader-resource and unordered-access states",
	"[buffer][state]")
{
	const spall::BufferCreateInfo info = bufferCreateInfo(
		spall::BufferUsageFlags::Storage,
		spall::ResourceStateFlags::ShaderResource | spall::ResourceStateFlags::UnorderedAccess);

	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"An indirect buffer must be device local",
	"[buffer][indirect]")
{
	spall::BufferCreateInfo info = {};
	info.Size = 256;
	info.Usage = spall::BufferUsageFlags::Indirect;

	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);

	info.CpuAccess = spall::MemoryAccess::Write;
	CHECK(spall::validateBufferCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);

	info.CpuAccess = spall::MemoryAccess::Read;
	CHECK(spall::validateBufferCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"The indirect-argument state requires indirect usage",
	"[buffer][indirect][state]")
{
	spall::BufferCreateInfo info = {};
	info.Size = 256;
	info.Usage = spall::BufferUsageFlags::Storage;
	info.InitialState = spall::ResourceStateFlags::IndirectArgument;

	CHECK(spall::validateBufferCreateInfo(info) == spall::ERR_INVALID_RESOURCE_STATE);

	info.Usage = spall::BufferUsageFlags::Indirect;
	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"Acceleration-structure-input usage serves the shader-resource state",
	"[buffer][acceleration][state]")
{
	spall::BufferCreateInfo info = bufferCreateInfo(
		spall::BufferUsageFlags::AccelerationStructureInput,
		spall::ResourceStateFlags::ShaderResource);

	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);

	info.CpuAccess = spall::MemoryAccess::Write;
	CHECK(spall::validateBufferCreateInfo(info) == spall::SUCCESS);

	info.CpuAccess = spall::MemoryAccess::Read;
	CHECK(spall::validateBufferCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);
}
