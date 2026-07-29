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
		REQUIRE(spall::dxgi::wideDebugLabel("Lighting", &wideLabel) == spall::SUCCESS);

		CHECK(wideLabel == L"Lighting");
		CHECK(spall::dxgi::eventPayloadSize(wideLabel) == (9 * sizeof(wchar_t)));
	}

	SECTION("a multi-byte label converts to its wide form")
	{
		std::wstring wideLabel;
		REQUIRE(spall::dxgi::wideDebugLabel("\xC3\xA9"
											"clairage",
					&wideLabel) == spall::SUCCESS);

		const std::wstring expected = std::wstring(1, static_cast<wchar_t>(0x00E9)) + L"clairage";

		CHECK(wideLabel == expected);
		CHECK(spall::dxgi::eventPayloadSize(wideLabel) == (10 * sizeof(wchar_t)));
	}

	SECTION("an empty label still carries a terminator")
	{
		std::wstring wideLabel;
		REQUIRE(spall::dxgi::wideDebugLabel("", &wideLabel) == spall::SUCCESS);

		CHECK(wideLabel.empty());
		CHECK(spall::dxgi::eventPayloadSize(wideLabel) == sizeof(wchar_t));
	}

	SECTION("invalid UTF-8 is rejected")
	{
		std::wstring wideLabel;
		CHECK(spall::dxgi::wideDebugLabel("\xFF\xFE", &wideLabel) != spall::SUCCESS);
	}

	SECTION("the unicode payload version is the one the event API decodes")
	{
		CHECK(spall::dxgi::UnicodeEventVersion == 0);
		CHECK(spall::dxgi::AnsiEventVersion == 1);
	}
}
