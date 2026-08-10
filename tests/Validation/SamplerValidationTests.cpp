#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/SamplerValidation.h>

#include <limits>

static constexpr spall::Filter EveryFilter[] = {spall::Filter::Nearest, spall::Filter::Linear};
static constexpr spall::AddressMode EveryAddressMode[] = {spall::AddressMode::ClampToEdge, spall::AddressMode::Repeat};
static constexpr spall::CompareOp EveryCompareOp[] = {
	spall::CompareOp::Never,
	spall::CompareOp::Less,
	spall::CompareOp::Equal,
	spall::CompareOp::LessOrEqual,
	spall::CompareOp::Greater,
	spall::CompareOp::NotEqual,
	spall::CompareOp::GreaterOrEqual,
	spall::CompareOp::Always};

static constexpr spall::Filter InvalidFilter = static_cast<spall::Filter>(99);
static constexpr spall::AddressMode InvalidAddressMode = static_cast<spall::AddressMode>(99);
static constexpr spall::CompareOp InvalidCompareOp = static_cast<spall::CompareOp>(99);

TEST_CASE(
	"A default sampler is valid",
	"[sampler]")
{
	const spall::SamplerCreateInfo info = {};

	CHECK(info.MinFilter == spall::Filter::Linear);
	CHECK(info.MagFilter == spall::Filter::Linear);
	CHECK_FALSE(info.ComparisonEnabled);
	CHECK(info.Comparison == spall::CompareOp::LessOrEqual);
	CHECK(info.AddressModeU == spall::AddressMode::Repeat);
	CHECK(spall::validateSamplerCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A comparison sampler accepts every comparison operation",
	"[sampler][comparison]")
{
	for (const spall::CompareOp comparison : EveryCompareOp)
	{
		spall::SamplerCreateInfo info = {};
		info.ComparisonEnabled = true;
		info.Comparison = comparison;

		CHECK(spall::validateSamplerCreateInfo(info) == spall::SUCCESS);
	}
}

TEST_CASE(
	"A sampler validates its comparison operation only when enabled",
	"[sampler][comparison]")
{
	spall::SamplerCreateInfo info = {};
	info.Comparison = InvalidCompareOp;

	CHECK(spall::validateSamplerCreateInfo(info) == spall::SUCCESS);

	info.ComparisonEnabled = true;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Every filter and address-mode combination is accepted",
	"[sampler]")
{
	for (const spall::Filter minFilter : EveryFilter)
	{
		for (const spall::Filter magFilter : EveryFilter)
		{
			for (const spall::AddressMode u : EveryAddressMode)
			{
				for (const spall::AddressMode v : EveryAddressMode)
				{
					for (const spall::AddressMode w : EveryAddressMode)
					{
						spall::SamplerCreateInfo info = {};
						info.MinFilter = minFilter;
						info.MagFilter = magFilter;
						info.AddressModeU = u;
						info.AddressModeV = v;
						info.AddressModeW = w;

						CHECK(spall::validateSamplerCreateInfo(info) == spall::SUCCESS);
					}
				}
			}
		}
	}
}

TEST_CASE(
	"A sampler rejects an invalid minification filter",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.MinFilter = InvalidFilter;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A sampler rejects an invalid magnification filter",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.MagFilter = InvalidFilter;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A sampler rejects an invalid mip filter",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.MipFilter = InvalidFilter;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A sampler anisotropy below one is rejected",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.MaxAnisotropy = 0.5f;
	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);

	info.MaxAnisotropy = std::numeric_limits<float>::quiet_NaN();
	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);

	info.MaxAnisotropy = 16.0f;
	CHECK(spall::validateSamplerCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A sampler level-of-detail range must be ordered",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.MinLod = 4.0f;
	info.MaxLod = 2.0f;
	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);

	info.MaxLod = 4.0f;
	CHECK(spall::validateSamplerCreateInfo(info) == spall::SUCCESS);

	info.MinLod = -1.0f;
	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);

	info.MinLod = std::numeric_limits<float>::quiet_NaN();
	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A sampler rejects an invalid U address mode",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.AddressModeU = InvalidAddressMode;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A sampler rejects an invalid V address mode",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.AddressModeV = InvalidAddressMode;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A sampler rejects an invalid W address mode",
	"[sampler]")
{
	spall::SamplerCreateInfo info = {};
	info.AddressModeW = InvalidAddressMode;

	CHECK(spall::validateSamplerCreateInfo(info) != spall::SUCCESS);
}
