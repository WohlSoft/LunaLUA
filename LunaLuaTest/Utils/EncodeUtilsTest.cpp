#include <catch.hpp>

#include <string>
#include <Utils/EncodeUtils.h>

// For BSTR
#include <WTypes.h>

TEST_CASE("UTF-8 chars to UTF-16", "[lunalua-utils-encode]") {
    // цдь
    const char* char_utf8 = u8"цдь";
    std::wstring wchar_utf8 = LunaLua::EncodeUtils::Str2WStr(std::string_view(char_utf8));
    
    REQUIRE(wchar_utf8.length() == 3u);
    REQUIRE(wchar_utf8 == L"цдь");
}

TEST_CASE("UTF16 chars to UTF-8", "[lunalua-utils-encode]")
{
    const wchar_t* wchar_utf16 = L"цдь";
    std::string char_utf8 = LunaLua::EncodeUtils::WStr2Str(std::wstring_view(wchar_utf16));

    REQUIRE(char_utf8.length() == 6u);
    REQUIRE(char_utf8 == u8"цдь");

    // Also test BMP
    const wchar_t* bmp_wchar_utf16 = L"А";
    std::string bmp_char_utf8 = LunaLua::EncodeUtils::WStr2Str(std::wstring_view(bmp_wchar_utf16));

    REQUIRE(bmp_char_utf8.length() == 3u);
    REQUIRE(bmp_char_utf8 == u8"А");
}

TEST_CASE("ANSI chars to UTF-16", "[lunalua-utils-encode]")
{
    const char* char_ansi = "€€€";
    std::wstring wchar_utf16 = LunaLua::EncodeUtils::StrA2WStr(std::string_view(char_ansi));
    REQUIRE(wchar_utf16.length() == 3u);
    REQUIRE(wchar_utf16 == L"€€€");
}

TEST_CASE("UTF-16 chars to ANSI", "[lunalua-utils-encode]")
{
    const wchar_t* wchar_utf16 = L"€€€";
    std::string char_ansi = LunaLua::EncodeUtils::WStr2StrA(std::wstring_view(wchar_utf16));
    REQUIRE(char_ansi.length() == 3u);
    REQUIRE(char_ansi == "€€€");

}

TEST_CASE("BSTR to ANSI", "[lunalua-utils-encode]")
{
    BSTR bstr_utf16 = SysAllocStringLen(L"€€€", 3);
    std::string char_ansi = LunaLua::EncodeUtils::BSTR2AStr(bstr_utf16);
    REQUIRE(char_ansi.length() == 3u);
    REQUIRE(char_ansi == "€€€");
}

TEST_CASE("Encode URL", "[lunalua-utils-encode]")
{
    std::string_view textToEncode = "Hello World!";
    std::wstring_view wideTextToEncode = L"Hello World in Wide!";

    std::string out = LunaLua::EncodeUtils::EncodeUrl(textToEncode);
    std::wstring out_wide = LunaLua::EncodeUtils::EncodeUrl(wideTextToEncode);

    REQUIRE(out == "Hello%20World%21");
    REQUIRE(out_wide == L"Hello%20World%20in%20Wide%21");
}

TEST_CASE("Test Empty Strings", "[lunalua-utils-encode]")
{
	// ANSI to UTF-16
	{
		const char* char_ansi = "";
		std::wstring wchar_utf16 = LunaLua::EncodeUtils::StrA2WStr(std::string_view(char_ansi));
		REQUIRE(wchar_utf16.length() == 0u);
	}

	// ANSI to UTF-16
	{
		const char* char_utf8 = u8"";
		std::wstring wchar_utf16 = LunaLua::EncodeUtils::Str2WStr(std::string_view(char_utf8));
		REQUIRE(wchar_utf16.length() == 0u);
	}

	// UTF-16 to ANSI
	{
		const wchar_t* wchar_utf16 = L"";
		std::string char_ansi = LunaLua::EncodeUtils::WStr2StrA(std::wstring_view(wchar_utf16));
		REQUIRE(char_ansi.length() == 0u);
	}

	// UTF-16 to ANSI
	{
		const wchar_t* wchar_utf16 = L"";
		std::string char_utf8 = LunaLua::EncodeUtils::WStr2Str(std::wstring_view(wchar_utf16));
		REQUIRE(char_utf8.length() == 0u);
	}

	// BSTR to ANSI
	{
		BSTR bstr_utf16 = SysAllocStringLen(L"", 0);
		std::string char_ansi = LunaLua::EncodeUtils::BSTR2AStr(bstr_utf16);
		REQUIRE(char_ansi.length() == 0u);
	}

	// URL Encode
	{
		std::string_view textToEncode = "";
		std::string out = LunaLua::EncodeUtils::EncodeUrl(textToEncode);
		REQUIRE(out.length() == 0u);
	}
}
