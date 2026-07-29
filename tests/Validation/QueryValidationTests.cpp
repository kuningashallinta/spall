#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/QueryValidation.h>
#include <tests/Support/Fakes.h>

#include <spall/Device/DeviceLimits.h>

#include <cstdint>
#include <limits>

TEST_CASE(
	"A query pool requires at least one timestamp",
	"[query][create]")
{
	spall::QueryPoolCreateInfo info = {};

	CHECK(spall::validateQueryPoolCreateInfo(info) == spall::ERR_INVALID_SIZE);

	info.TimestampCount = 1;
	CHECK(spall::validateQueryPoolCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A timestamp write stays inside its query pool",
	"[query][write]")
{
	const spall::QueryPoolInfo info = spall::tests::queryPoolInfo(8);

	CHECK(spall::validateTimestampWrite(info, 0) == spall::SUCCESS);
	CHECK(spall::validateTimestampWrite(info, 7) == spall::SUCCESS);
	CHECK(spall::validateTimestampWrite(info, 8) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateTimestampWrite(info, 4000) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A timestamp read stays inside its query pool",
	"[query][read]")
{
	const spall::QueryPoolInfo info = spall::tests::queryPoolInfo(8);

	CHECK(spall::validateTimestampRead(info, 0, 8) == spall::SUCCESS);
	CHECK(spall::validateTimestampRead(info, 6, 2) == spall::SUCCESS);
	CHECK(spall::validateTimestampRead(info, 7, 2) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateTimestampRead(info, 8, 1) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A timestamp read rejects an empty range",
	"[query][read]")
{
	const spall::QueryPoolInfo info = spall::tests::queryPoolInfo(8);

	CHECK(spall::validateTimestampRead(info, 0, 0) == spall::ERR_INVALID_SIZE);
	CHECK(spall::validateTimestampRead(info, 8, 0) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A timestamp read range cannot overflow its bounds check",
	"[query][read]")
{
	const spall::QueryPoolInfo info = spall::tests::queryPoolInfo(8);
	const std::uint32_t maximum = (std::numeric_limits<std::uint32_t>::max)();

	CHECK(spall::validateTimestampRead(info, maximum, 8) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateTimestampRead(info, 1, maximum) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"Timestamp support defaults to unavailable",
	"[query][capabilities]")
{
	const spall::DeviceLimits limits = {};
	const spall::QueryPoolInfo info = {};

	CHECK_FALSE(limits.SupportsTimestampQueries);
	CHECK(info.TimestampCount == 0);
	CHECK(info.DebugName == nullptr);
}
