#include <fea_getopt/fea_getopt.hpp>
#include <fea_utils/fea_utils.hpp>
#include <gtest/gtest.h>

#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace {
std::filesystem::path exe_path;

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
std::pair<size_t, const CharT**> make_print_help() {
	static std::vector<const CharT*> opts = {
		FEA_ML("tool.exe"),
		FEA_ML("-h"),
	};

	return { opts.size(), opts.data() };
}

template <class CharT>
std::pair<size_t, const CharT**> make_test_options() {
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
		fea::get_opt<char> opt;
		add_options(opt);

		{
			auto [argc, argv] = make_print_help<char>();
			opt.parse_options(argc, argv);
		}

		//{
		//	auto [argc, argv] = make_test_options<char>();
		//	opt.parse_options(argc, argv);
		//}
	}

	{
		fea::get_opt<wchar_t> opt;
		add_options(opt);

		{
			auto [argc, argv] = make_print_help<wchar_t>();
			opt.parse_options(argc, argv);
		}
	}

	{
		fea::get_opt<char16_t> opt;
		add_options(opt);

		{
			auto [argc, argv] = make_print_help<char16_t>();
			opt.parse_options(argc, argv);
		}
	}

	{
		fea::get_opt<char32_t> opt;
		add_options(opt);

		{
			auto [argc, argv] = make_print_help<char32_t>();
			opt.parse_options(argc, argv);
		}
	}
}


} // namespace

int main(int argc, char** argv) {
	exe_path = fea::executable_dir(argv[0]);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
