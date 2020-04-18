#include <fea_getopt/fea_getopt.hpp>
#include <fea_utils/fea_utils.hpp>
#include <gtest/gtest.h>

#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace {
std::filesystem::path exe_path;

TEST(fea_getopt, basics) {

#if defined(_MSC_VER)
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	std::string u8 = "\u00df\u6c34\U0001f34c";
	std::wstring u16 = L"\u00df\u6c34\U0001f34c";
	printf("%s\n", u8.c_str());
	wprintf(L"%s\n", u16.c_str());

	{
		fea::get_opt<char> opt;
		opt.print("This should compile and not %s.\n", "throw");

		const char* fmt = FEA_MAKE_LITERAL_T(char, "%s\n");
		const char* test = FEA_MAKE_LITERAL_T(char, "test char");
		opt.print(fmt, test);
	}

	{
		fea::get_opt<wchar_t> opt;
		opt.print(L"This should compile and not %s.\n", L"throw");

		const wchar_t* fmt = FEA_MAKE_LITERAL_T(wchar_t, "%s\n");
		const wchar_t* test = FEA_MAKE_LITERAL_T(wchar_t, "test wchar");
		opt.print(fmt, test);
	}

	{
		fea::get_opt<char16_t> opt;
		opt.print(u"This should compile and not %s.\n", u"throw");

		const char16_t* test = FEA_MAKE_LITERAL_T(char16_t, "test char16");
		opt.print(u"%s\n", test);
	}

	{
		fea::get_opt<char32_t> opt;
		opt.print(U"This should compile and not %s.\n", U"throw");

		const char32_t* test = FEA_MAKE_LITERAL_T(char32_t, "test char32");
		opt.print(U"%s\n", test);
	}
}


} // namespace

int main(int argc, char** argv) {
	exe_path = fea::executable_dir(argv[0]);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
