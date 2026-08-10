#include <catch2/catch_test_macros.hpp>

#include <src/Common/DXGI/DXGIDebugLabel.h>

#include <string>

TEST_CASE(
	"A DXGI debug label converts from UTF-8 and sizes its event payload",
	"[dxgi][debugmarker]")
{
	SECTION("an ascii label keeps its text")
	{
		std::wstring wideLabel;
		REQUIRE(spall::d3d12::wideDebugLabel("Lighting", &wideLabel) == spall::SUCCESS);

		CHECK(wideLabel == L"Lighting");
		CHECK(spall::d3d12::eventPayloadSize(wideLabel) == (9 * sizeof(wchar_t)));
	}

	SECTION("a multi-byte label converts to its wide form")
	{
		std::wstring wideLabel;
		REQUIRE(spall::d3d12::wideDebugLabel("\xC3\xA9"
											"clairage",
					&wideLabel) == spall::SUCCESS);

		const std::wstring expected = std::wstring(1, static_cast<wchar_t>(0x00E9)) + L"clairage";

		CHECK(wideLabel == expected);
		CHECK(spall::d3d12::eventPayloadSize(wideLabel) == (10 * sizeof(wchar_t)));
	}

	SECTION("an empty label still carries a terminator")
	{
		std::wstring wideLabel;
		REQUIRE(spall::d3d12::wideDebugLabel("", &wideLabel) == spall::SUCCESS);

		CHECK(wideLabel.empty());
		CHECK(spall::d3d12::eventPayloadSize(wideLabel) == sizeof(wchar_t));
	}

	SECTION("invalid UTF-8 is rejected")
	{
		std::wstring wideLabel;
		CHECK(spall::d3d12::wideDebugLabel("\xFF\xFE", &wideLabel) != spall::SUCCESS);
	}

	SECTION("the unicode payload version is the one the event API decodes")
	{
		CHECK(spall::d3d12::UnicodeEventVersion == 0);
		CHECK(spall::d3d12::AnsiEventVersion == 1);
	}
}
