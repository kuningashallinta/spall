#include <catch2/catch_test_macros.hpp>

#include <spall/Common/Bit.h>
#include <spall/Common/Enums/BufferUsageFlags.h>
#include <spall/Common/Enums/TextureUsageFlags.h>
#include <src/Validation/Common/FlagValidation.h>

enum class TestFlags : std::uint32_t
{
	None = 0,
	First = BIT(0),
	Second = BIT(1),
	Third = BIT(2),
	High = BIT(31)
};

ENUM_CLASS_BITWISE_OPERATORS(
	TestFlags)

static_assert(spall::hasOnlyKnownFlags(TestFlags::First, TestFlags::First | TestFlags::Second));
static_assert(not spall::hasOnlyKnownFlags(TestFlags::Third, TestFlags::First | TestFlags::Second));
static_assert(spall::hasAnyFlag(TestFlags::First | TestFlags::Second, TestFlags::Second));
static_assert(not spall::hasAnyFlag(TestFlags::First, TestFlags::Second));

TEST_CASE(
	"No flags are only known flags",
	"[flags]")
{
	CHECK(spall::hasOnlyKnownFlags(TestFlags::None, TestFlags::First));
	CHECK(spall::hasOnlyKnownFlags(TestFlags::None, TestFlags::None));
}

TEST_CASE(
	"A subset of the known flags is accepted",
	"[flags]")
{
	constexpr TestFlags known = TestFlags::First | TestFlags::Second | TestFlags::Third;

	CHECK(spall::hasOnlyKnownFlags(TestFlags::First, known));
	CHECK(spall::hasOnlyKnownFlags(TestFlags::First | TestFlags::Third, known));
	CHECK(spall::hasOnlyKnownFlags(known, known));
}

TEST_CASE(
	"Any flag outside the known set is rejected",
	"[flags]")
{
	constexpr TestFlags known = TestFlags::First | TestFlags::Second;

	CHECK_FALSE(spall::hasOnlyKnownFlags(TestFlags::Third, known));
	CHECK_FALSE(spall::hasOnlyKnownFlags(TestFlags::First | TestFlags::Third, known));
	CHECK_FALSE(spall::hasOnlyKnownFlags(TestFlags::High, known));
}

TEST_CASE(
	"Only known flags survives the highest bit",
	"[flags]")
{
	CHECK(spall::hasOnlyKnownFlags(TestFlags::High, TestFlags::High));
	CHECK(spall::hasOnlyKnownFlags(TestFlags::First | TestFlags::High, TestFlags::First | TestFlags::High));
	CHECK_FALSE(spall::hasOnlyKnownFlags(TestFlags::First | TestFlags::High, TestFlags::First));
}

TEST_CASE(
	"Any flag reports an overlap",
	"[flags]")
{
	CHECK(spall::hasAnyFlag(TestFlags::First, TestFlags::First));
	CHECK(spall::hasAnyFlag(TestFlags::First | TestFlags::Second, TestFlags::Second));
	CHECK(spall::hasAnyFlag(TestFlags::First | TestFlags::Second, TestFlags::Second | TestFlags::Third));
}

TEST_CASE(
	"Any flag reports no overlap",
	"[flags]")
{
	CHECK_FALSE(spall::hasAnyFlag(TestFlags::First, TestFlags::Second));
	CHECK_FALSE(spall::hasAnyFlag(TestFlags::None, TestFlags::First));
	CHECK_FALSE(spall::hasAnyFlag(TestFlags::First, TestFlags::None));
	CHECK_FALSE(spall::hasAnyFlag(TestFlags::None, TestFlags::None));
}

TEST_CASE(
	"Any flag is an overlap test, not a containment test",
	"[flags]")
{
	CHECK(spall::hasAnyFlag(TestFlags::First, TestFlags::First | TestFlags::Second));
}

TEST_CASE(
	"The flag helpers work on the real usage enums",
	"[flags]")
{
	constexpr spall::BufferUsageFlags knownBufferUsage = spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index | spall::BufferUsageFlags::Storage;

	CHECK(spall::hasOnlyKnownFlags(spall::BufferUsageFlags::Vertex, knownBufferUsage));
	CHECK_FALSE(spall::hasOnlyKnownFlags(spall::BufferUsageFlags::Uniform, knownBufferUsage));
	CHECK(spall::hasAnyFlag(spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index, spall::BufferUsageFlags::Index));

	CHECK(spall::hasOnlyKnownFlags(spall::TextureUsageFlags::Sampled, spall::TextureUsageFlags::Sampled));
	CHECK_FALSE(spall::hasAnyFlag(spall::TextureUsageFlags::Sampled, spall::TextureUsageFlags::ColorAttachment));
}
