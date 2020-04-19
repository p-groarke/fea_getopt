#include <fea_getopt/fea_getopt.hpp>
#include <fea_utils/fea_utils.hpp>
#include <gtest/gtest.h>

#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace {
std::filesystem::path exe_path;
std::string last_printed_help(2048, '\0');
std::wstring wlast_printed_help(2048, L'\0');

#pragma warning(push)
#pragma warning(disable : 4996)
int print_to_string(const char* fmt, ...) {
	last_printed_help.clear();
	va_list args;
	va_start(args, fmt);
	int ret = vsprintf(last_printed_help.data(), fmt, args);
	va_end(args);
	return ret;
}
int print_to_string16(const char16_t* fmt, ...) {
	std::string utf8 = fea::utf16_to_utf8({ fmt });

	last_printed_help.clear();
	va_list args;
	va_start(args, fmt);
	int ret = vsprintf(last_printed_help.data(), utf8.c_str(), args);
	va_end(args);
	return ret;
}
int print_to_string32(const char32_t* fmt, ...) {
	std::string utf8 = fea::utf32_to_utf8({ fmt });

	last_printed_help.clear();
	va_list args;
	va_start(args, fmt);
	int ret = vsprintf(last_printed_help.data(), utf8.c_str(), args);
	va_end(args);
	return ret;
}
#pragma warning(pop)

int wprint_to_string(const wchar_t* fmt, ...) {
	wlast_printed_help.clear();
	va_list args;
	va_start(args, fmt);
	int ret = wvsprintf(wlast_printed_help.data(), fmt, args);
	va_end(args);
	return ret;
}


template <class CharT, class PrintFunc>
void add_options(fea::get_opt<CharT, PrintFunc>& opts) {
	using string_t = std::basic_string<CharT, std::char_traits<CharT>,
			std::allocator<CharT>>;

	opts.add_raw_option(
			FEA_ML("filename"),
			[](string_t&& str) {
				static_assert(
						std::is_same_v<typename string_t::value_type, CharT>,
						"unit test failed : Called with string must be same "
						"type as getopt.");

				string_t cmp = FEA_ML("some raw arg");
				EXPECT_EQ(str, cmp);
				fea::detail::any_print(str.c_str());

				return true;
			},
			FEA_ML("File to process.\nThis is a second indented string.\nAnd a "
				   "third."));

	opts.add_raw_option(
			FEA_ML("other_raw_opt"),
			[](string_t&& str) {
				static_assert(
						std::is_same_v<typename string_t::value_type, CharT>,
						"unit test failed : Called with string must be same "
						"type as getopt.");

				fea::detail::any_print(str.c_str());

				return true;
			},
			FEA_ML("Some looooooooong string that should be cut off by the "
				   "library and reindented appropriately. Hopefully without "
				   "splitting inside a word and making everything super nice "
				   "for users that can even add backslash n if they want to "
				   "start another sentence at the right indentantation like "
				   "this following sentence.\nI am a sentence that should "
				   "start at a newline, but still be split appropriately if I "
				   "am too long because that would be unfortunate wouldn't it "
				   "now."));

	opts.add_flag_option(
			FEA_ML("flag"), []() { return true; }, FEA_ML("A simple flag."),
			FEA_CH('f'));

	opts.add_default_arg_option(
			FEA_ML("default_arg"),
			[](string_t&& str) {
				static_assert(
						std::is_same_v<typename string_t::value_type, CharT>,
						"unit test failed : Called with string must be same "
						"type as getopt.");

				fea::detail::any_print(str.c_str());

				return true;
			},
			FEA_ML("Some looooooooong string that should be cut off by the "
				   "library and reindented appropriately. Hopefully without "
				   "splitting inside a word and making everything super nice "
				   "for users that can even add backslash n if they want to "
				   "start another sentence at the right indentantation like "
				   "this following sentence.\nI am a sentence that should "
				   "start at a newline, but still be split appropriately if I "
				   "am too long because that would be unfortunate wouldn't it "
				   "now."),
			FEA_ML("d_val"), FEA_CH('d'));
}

template <class CharT>
std::pair<size_t, const CharT**> test_help() {
	static std::vector<const CharT*> opts = {
		FEA_ML("tool.exe"),
		FEA_ML("-h"),
	};

	return { opts.size(), opts.data() };
}

template <class CharT>
std::pair<size_t, const CharT**> test_raw() {
	static std::vector<const CharT*> opts = {
		FEA_ML("tool.exe"),
		FEA_ML("some raw arg"),
	};

	return { opts.size(), opts.data() };
}

TEST(getopt, printing) {

#if defined(_MSC_VER)
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	// std::string u8 = "\u00df\u6c34\U0001f34c";
	// std::wstring u16 = L"\u00df\u6c34\U0001f34c";
	// printf("%s\n", u8.c_str());
	// wprintf(L"%s\n", u16.c_str());

	std::string test2 = "my test";
	printf(test2.c_str());
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

		const char32_t* test = FEA_MAKE_LITERAL_T(char32_t, "test char32");
		opt.print(U"%s\n", test);
	}
}

TEST(getopt, basics) {


	{
		fea::get_opt<char> opt{ print_to_string };
		add_options(opt);

		{
			auto [argc, argv] = test_help<char>();
			opt.parse_options(argc, argv);
		}

		{
			auto [argc, argv] = test_raw<char>();
			opt.parse_options(argc, argv);
			// EXPECT_EQ(last_printed_help, std::string{ argv[1] });
		}
	}

	{
		fea::get_opt<wchar_t> opt{ wprint_to_string };
		add_options(opt);

		{
			auto [argc, argv] = test_help<wchar_t>();
			opt.parse_options(argc, argv);
		}

		{
			auto [argc, argv] = test_raw<wchar_t>();
			opt.parse_options(argc, argv);
			// EXPECT_EQ(last_printed_help, std::string{ argv[1] });
		}
	}

	{
		fea::get_opt<char16_t> opt{ print_to_string16 };
		add_options(opt);

		{
			auto [argc, argv] = test_help<char16_t>();
			opt.parse_options(argc, argv);
		}

		{
			auto [argc, argv] = test_raw<char16_t>();
			opt.parse_options(argc, argv);
			// EXPECT_EQ(last_printed_help, std::string{ argv[1] });
		}
	}

	{
		fea::get_opt<char32_t> opt{ print_to_string32 };
		add_options(opt);

		{
			auto [argc, argv] = test_help<char32_t>();
			opt.parse_options(argc, argv);
		}

		{
			auto [argc, argv] = test_raw<char32_t>();
			opt.parse_options(argc, argv);
			// EXPECT_EQ(last_printed_help, std::string{ argv[1] });
		}
	}
}


} // namespace

int main(int argc, char** argv) {
	exe_path = fea::executable_dir(argv[0]);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
