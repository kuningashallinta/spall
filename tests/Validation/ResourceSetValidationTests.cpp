#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/ResourceSetValidation.h>
#include <tests/Support/Fakes.h>

#include <array>
#include <span>

static spall::ResourceBindingInfo binding(
	std::uint32_t slot,
	spall::ResourceBindingType type,
	spall::ShaderStageFlags stages)
{
	spall::ResourceBindingInfo info = {};
	info.Binding = slot;
	info.Type = type;
	info.Stages = stages;

	return info;
}

static spall::ResourceSetLayoutCreateInfo layout(
	std::span<const spall::ResourceBindingInfo> bindings)
{
	spall::ResourceSetLayoutCreateInfo info = {};
	info.Bindings = bindings;

	return info;
}

TEST_CASE(
	"A layout requires at least one binding",
	"[resourceset]")
{
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout({})) != spall::SUCCESS);
}

TEST_CASE(
	"A layout accepts a uniform buffer in a graphics stage",
	"[resourceset]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(0, spall::ResourceBindingType::UniformBuffer, spall::ShaderStageFlags::Vertex | spall::ShaderStageFlags::Fragment)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) == spall::SUCCESS);
}

TEST_CASE(
	"A layout rejects duplicate binding slots",
	"[resourceset]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(2, spall::ResourceBindingType::UniformBuffer, spall::ShaderStageFlags::Vertex),
		binding(2, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Fragment)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) != spall::SUCCESS);
}

TEST_CASE(
	"A layout accepts distinct binding slots",
	"[resourceset]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(0, spall::ResourceBindingType::UniformBuffer, spall::ShaderStageFlags::Vertex),
		binding(1, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Fragment)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) == spall::SUCCESS);
}

TEST_CASE(
	"A layout rejects an unsupported binding type",
	"[resourceset]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(0, static_cast<spall::ResourceBindingType>(99), spall::ShaderStageFlags::Vertex)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"A layout binding requires at least one shader stage",
	"[resourceset]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(0, spall::ResourceBindingType::UniformBuffer, spall::ShaderStageFlags::None)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) != spall::SUCCESS);
}

TEST_CASE(
	"A layout binding rejects an unknown shader stage",
	"[resourceset]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(0, spall::ResourceBindingType::UniformBuffer, static_cast<spall::ShaderStageFlags>(1u << 20))};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) != spall::SUCCESS);
}

TEST_CASE(
	"A storage buffer binding allows the compute and fragment stages",
	"[resourceset]")
{
	const spall::ResourceBindingInfo compute[] = {
		binding(0, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::Compute)};
	const spall::ResourceBindingInfo fragment[] = {
		binding(0, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::Fragment)};
	const spall::ResourceBindingInfo both[] = {
		binding(0, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::Compute | spall::ShaderStageFlags::Fragment)};
	const spall::ResourceBindingInfo vertex[] = {
		binding(0, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::Compute | spall::ShaderStageFlags::Vertex)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(compute)) == spall::SUCCESS);
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(fragment)) == spall::SUCCESS);
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(both)) == spall::SUCCESS);
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(vertex)) != spall::SUCCESS);
}

TEST_CASE(
	"A storage texture binding allows the compute and fragment stages",
	"[resourceset][storage][texture]")
{
	const spall::ResourceBindingInfo compute[] = {
		binding(0, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute)};
	const spall::ResourceBindingInfo fragment[] = {
		binding(0, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Fragment)};
	const spall::ResourceBindingInfo both[] = {
		binding(0, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute | spall::ShaderStageFlags::Fragment)};
	const spall::ResourceBindingInfo vertex[] = {
		binding(0, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Vertex)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(compute)) == spall::SUCCESS);
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(fragment)) == spall::SUCCESS);
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(both)) == spall::SUCCESS);
	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(vertex)) == spall::ERR_UNSUPPORTED_SHADER_STAGE);
}

TEST_CASE(
	"A resource set requires a layout",
	"[resourceset]")
{
	spall::ResourceSetCreateInfo info = {};

	CHECK(spall::validateResourceSetCreateInfo(info) != spall::SUCCESS);

	FakeResourceSetLayout layout;
	info.Layout = &layout;

	CHECK(spall::validateResourceSetCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"Resource-set writes cannot be empty",
	"[resourceset]")
{
	CHECK(spall::validateResourceWrites({}) != spall::SUCCESS);
}

TEST_CASE(
	"A uniform-buffer write requires a buffer",
	"[resourceset]")
{
	FakeBuffer buffer(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Uniform});

	spall::ResourceWrite withBuffer = {};
	withBuffer.Type = spall::ResourceBindingType::UniformBuffer;
	withBuffer.Buffer = &buffer;

	spall::ResourceWrite withoutBuffer = {};
	withoutBuffer.Type = spall::ResourceBindingType::UniformBuffer;

	const std::array<spall::ResourceWrite, 1> present = {withBuffer};
	const std::array<spall::ResourceWrite, 1> missing = {withoutBuffer};

	CHECK(spall::validateResourceWrites(present) == spall::SUCCESS);
	CHECK(spall::validateResourceWrites(missing) != spall::SUCCESS);
}

TEST_CASE(
	"A storage-buffer write requires a buffer",
	"[resourceset]")
{
	spall::ResourceWrite write = {};
	write.Type = spall::ResourceBindingType::StorageBuffer;

	const std::array<spall::ResourceWrite, 1> writes = {write};

	CHECK(spall::validateResourceWrites(writes) != spall::SUCCESS);
}

TEST_CASE(
	"A storage-buffer write accepts a buffer",
	"[resourceset]")
{
	FakeBuffer buffer(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Storage});

	spall::ResourceWrite write = {};
	write.Type = spall::ResourceBindingType::StorageBuffer;
	write.Buffer = &buffer;

	CHECK(spall::validateResourceWrites(std::array {write}) == spall::SUCCESS);
}

TEST_CASE(
	"Resource-set writes reject an unsupported binding type",
	"[resourceset]")
{
	spall::ResourceWrite write = {};
	write.Type = static_cast<spall::ResourceBindingType>(99);

	CHECK(spall::validateResourceWrites(std::array {write}) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Resource-set writes reject duplicate binding slots",
	"[resourceset]")
{
	FakeBuffer buffer(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Uniform});

	spall::ResourceWrite first = {};
	first.Binding = 1;
	first.Type = spall::ResourceBindingType::UniformBuffer;
	first.Buffer = &buffer;

	spall::ResourceWrite second = first;

	const std::array<spall::ResourceWrite, 2> writes = {first, second};

	CHECK(spall::validateResourceWrites(writes) != spall::SUCCESS);
}

TEST_CASE(
	"A sampled-texture write requires both a view and a sampler",
	"[resourceset]")
{
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);
	FakeSampler sampler;

	spall::ResourceWrite complete = {};
	complete.Type = spall::ResourceBindingType::SampledTexture;
	complete.TextureView = &view;
	complete.Sampler = &sampler;

	spall::ResourceWrite noSampler = complete;
	noSampler.Sampler = nullptr;

	spall::ResourceWrite noView = complete;
	noView.TextureView = nullptr;

	CHECK(spall::validateResourceWrites(std::array {complete}) == spall::SUCCESS);
	CHECK(spall::validateResourceWrites(std::array {noSampler}) != spall::SUCCESS);
	CHECK(spall::validateResourceWrites(std::array {noView}) != spall::SUCCESS);
}

TEST_CASE(
	"A sampled-texture write requires sampled usage on the texture",
	"[resourceset]")
{
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::ColorAttachment});
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);
	FakeSampler sampler;

	spall::ResourceWrite write = {};
	write.Type = spall::ResourceBindingType::SampledTexture;
	write.TextureView = &view;
	write.Sampler = &sampler;

	CHECK(spall::validateResourceWrites(std::array {write}) != spall::SUCCESS);
}

TEST_CASE(
	"A storage-texture write requires a single color mip and no sampler",
	"[resourceset][storage][texture]")
{
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 4,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Storage});
	FakeTextureView singleMip(texture, spall::TextureAspectFlags::Color, 2, 1);
	FakeTextureView mipRange(texture, spall::TextureAspectFlags::Color, 0, 4);
	FakeTextureView depthAspect(texture, spall::TextureAspectFlags::Depth);
	FakeSampler sampler;

	spall::ResourceWrite valid = {};
	valid.Type = spall::ResourceBindingType::StorageTexture;
	valid.TextureView = &singleMip;

	spall::ResourceWrite missingView = valid;
	missingView.TextureView = nullptr;
	spall::ResourceWrite withSampler = valid;
	withSampler.Sampler = &sampler;
	spall::ResourceWrite multipleMips = valid;
	multipleMips.TextureView = &mipRange;
	spall::ResourceWrite nonColor = valid;
	nonColor.TextureView = &depthAspect;

	CHECK(spall::validateResourceWrites(std::array {valid}) == spall::SUCCESS);
	CHECK(spall::validateResourceWrites(std::array {missingView}) != spall::SUCCESS);
	CHECK(spall::validateResourceWrites(std::array {withSampler}) != spall::SUCCESS);
	CHECK(spall::validateResourceWrites(std::array {multipleMips}) != spall::SUCCESS);
	CHECK(spall::validateResourceWrites(std::array {nonColor}) != spall::SUCCESS);
}

TEST_CASE(
	"A storage-texture write requires storage texture usage",
	"[resourceset][storage][texture]")
{
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);
	spall::ResourceWrite write = {};
	write.Type = spall::ResourceBindingType::StorageTexture;
	write.TextureView = &view;

	CHECK(spall::validateResourceWrites(std::array {write}) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"An acceleration-structure binding is allowed in every shader stage",
	"[resourceset][acceleration]")
{
	const spall::ResourceBindingInfo bindings[] = {
		binding(0, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::Vertex),
		binding(1, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::Fragment),
		binding(2, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::Compute)};

	CHECK(spall::validateResourceSetLayoutCreateInfo(layout(bindings)) == spall::SUCCESS);
}

TEST_CASE(
	"An acceleration-structure write requires an acceleration structure",
	"[resourceset][acceleration]")
{
	FakeAccelerationStructure structure(spall::AccelerationStructureInfo {
		.Type = spall::AccelerationStructureType::TopLevel,
		.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace,
		.Size = 1024,
		.BuildScratchSize = 512});

	spall::ResourceWrite write = {};
	write.Type = spall::ResourceBindingType::AccelerationStructure;

	CHECK(spall::validateResourceWrites(std::array {write}) == spall::ERR_INVALID_RESOURCE);

	write.AccelerationStructure = &structure;
	CHECK(spall::validateResourceWrites(std::array {write}) == spall::SUCCESS);
}
