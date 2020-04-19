#include <algorithm>
#include <array>
#include <fea_getopt/fea_getopt.hpp>
#include <fea_utils/fea_utils.hpp>
#include <gtest/gtest.h>
#include <random>

#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace {
std::filesystem::path exe_path;
std::string last_printed_string;
std::wstring last_printed_wstring;

constexpr bool do_console_print = false;

int print_to_string(const std::string& message) {
	last_printed_string = message;
	if constexpr (do_console_print) {
		return printf(message.c_str());
	} else {
		return 0;
	}
}
int print_to_wstring(const std::wstring& message) {
	last_printed_wstring = message;
	if constexpr (do_console_print) {
		return wprintf(message.c_str());
	} else {
		return 0;
	}
}
int print_to_string16(const std::u16string& message) {
	last_printed_string = fea::utf16_to_utf8(message);
	if constexpr (do_console_print) {
		return printf(last_printed_string.c_str());
	} else {
		return 0;
	}
}
int print_to_string32(const std::u32string& message) {
	last_printed_string = fea::utf32_to_utf8(message);
	if constexpr (do_console_print) {
		return printf(last_printed_string.c_str());
	} else {
		return 0;
	}
}

// Testing framework. Generates an object containing the "called with" options
// to unit test.

template <class CharT>
struct option_tester {
	using string = std::basic_string<CharT, std::char_traits<CharT>,
			std::allocator<CharT>>;

	enum class opt_type {
		arg0,
		help,
		raw,
		flag,
		default_arg,
		optional,
		required,
		multi,
		count,
	};

	struct test_case {
		opt_type type;
		bool expected = false;
		string passed_in_data;
		bool was_recieved = false;
		string recieved_data;
	};


	// Allways init arg0
	option_tester() {
		_data[size_t(opt_type::arg0)].push_back({
				opt_type::arg0,
				true,
				FEA_ML("tool.exe"),
				false,
				FEA_ML(""),
		});
	}

	void add_test(opt_type test_type, string&& passed_in_str) {
		assert(test_type != opt_type::count);
		_data[size_t(test_type)].push_back({
				test_type,
				true,
				std::move(passed_in_str),
				false,
				FEA_ML(""),
		});
	}

	void populate() {
		// Randomize options except for arg0.

		// Only supports 1 arg0.
		assert(_data[size_t(opt_type::arg0)].size() == 1);
		_expected_data.push_back(_data[size_t(opt_type::arg0)].back());

		// Gather all the options to be randomized.
		std::vector<test_case> temp;
		for (size_t i = 0; i < size_t(opt_type::count); ++i) {
			opt_type o = opt_type(i);
			if (o == opt_type::arg0) {
				continue;
			}

			temp.insert(temp.end(), _data[i].begin(), _data[i].end());
		}

		auto rng = std::mt19937_64{};
		std::shuffle(temp.begin(), temp.end(), rng);

		// Finally, since help will abort all the other options, we make sure
		// there are no test_cases after a help option.
		auto it = std::find_if(temp.begin(), temp.end(),
				[](const test_case& t) { return t.type == opt_type::help; });

		if (it != temp.end()) {
			temp.erase(it + 1, temp.end());
		}

		// Finally, generate the test scenario.
		_expected_data.insert(_expected_data.end(), temp.begin(), temp.end());
	}

	std::vector<const CharT*> get_argv() const {
		// Randomize options except for arg0.
		std::vector<const CharT*> ret;

		for (const test_case& t : _expected_data) {
			ret.push_back(t.passed_in_data.c_str());
		}
		return ret;
	}

	void recieved(opt_type test_type, string&& recieved_str) {
		// Find first not set.
		auto it = std::find_if(_expected_data.begin(), _expected_data.end(),
				[&](const test_case& t) {
					return t.type == test_type && t.was_recieved == false;
				});

		if (it == _expected_data.end()) {
			throw std::runtime_error{
				"test failed, recieved extra option that wasn't expected"
			};
			return;
		}

		it->was_recieved = true;
		it->recieved_data = std::move(recieved_str);
	}

	void testit() const {
		for (const test_case& t : _expected_data) {
			EXPECT_EQ(t.was_recieved, t.expected);
			EXPECT_EQ(t.recieved_data, t.passed_in_data);
		}
	}

private:
	std::array<std::vector<test_case>, size_t(opt_type::count)> _data;

	std::vector<test_case> _expected_data;
};


// These must be set by the test, so the callbacks can set the recieved data.
static option_tester<char> char_global_tester;
static option_tester<wchar_t> wchar_global_tester;
static option_tester<char16_t> char16_global_tester;
static option_tester<char32_t> char32_global_tester;

template <class CharT>
static constexpr auto& get_global_tester() {
	if constexpr (std::is_same_v<CharT, char>) {
		return char_global_tester;
	} else if constexpr (std::is_same_v<CharT, wchar_t>) {
		return wchar_global_tester;
	} else if constexpr (std::is_same_v<CharT, char16_t>) {
		return char16_global_tester;
	} else if constexpr (std::is_same_v<CharT, char32_t>) {
		return char32_global_tester;
	}
}


// Encapsulates many random ordered options. Basically a fuzzer.
template <class CharT>
struct test_scenario {
	using opt_test = option_tester<CharT>;

	void fuzzit(fea::get_opt<CharT>& opt) {
		for (opt_test& test : tests) {
			test.populate();
		}

		for (const opt_test& test : tests) {
			auto& g_tester = get_global_tester<CharT>();
			g_tester = test;

			std::vector<const CharT*> opts = g_tester.get_argv();
			opt.parse_options(opts.size(), opts.data());

			g_tester.testit();
		}
	}

	std::vector<option_tester<CharT>> tests;
};


// Generate a test for all supported help options.
template <class CharT>
test_scenario<CharT> test_all_help() {
	test_scenario<CharT> ret;

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::help, FEA_ML("-h"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::help, FEA_ML("--help"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::help, FEA_ML("/h"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::help, FEA_ML("/help"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::help, FEA_ML("/?"));

	return ret;
}

// Test 2 raw args.
template <class CharT>
test_scenario<CharT> test_2raw() {
	test_scenario<CharT> ret;

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 1"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 2"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 1"));
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 2"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 1"));
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 2"));

	ret.tests.push_back({});
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 1"));
	ret.tests.back().add_test(
			option_tester<CharT>::opt_type::raw, FEA_ML("raw arg 2"));

	return ret;
}

// Add all options compatible with the option_tester.
template <class CharT, class PrintFunc>
void add_options(fea::get_opt<CharT, PrintFunc>& opts) {
	using string_t = std::basic_string<CharT, std::char_traits<CharT>,
			std::allocator<CharT>>;

	opts.add_arg0_callback([](string_t&& str) {
		using opt_test = option_tester<CharT>;
		opt_test& t = get_global_tester<CharT>();
		t.recieved(opt_test::opt_type::arg0, std::move(str));
		return true;
	});

	opts.add_help_callback([](string_t&& h) {
		using opt_test = option_tester<CharT>;
		opt_test& t = get_global_tester<CharT>();
		t.recieved(opt_test::opt_type::help, std::move(h));
	});

	opts.add_raw_option(
			FEA_ML("filename"),
			[](string_t&& str) {
				static_assert(
						std::is_same_v<typename string_t::value_type, CharT>,
						"unit test failed : Called with string must be same "
						"type as getopt.");

				using opt_test = option_tester<CharT>;
				opt_test& t = get_global_tester<CharT>();
				t.recieved(opt_test::opt_type::raw, std::move(str));

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

				using opt_test = option_tester<CharT>;
				opt_test& t = get_global_tester<CharT>();
				t.recieved(opt_test::opt_type::raw, std::move(str));

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
			[](string_t&&) {
				static_assert(
						std::is_same_v<typename string_t::value_type, CharT>,
						"unit test failed : Called with string must be same "
						"type as getopt.");

				// TODO : test

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
		fea::get_opt<char> opt{ print_to_string };
		const char* test = FEA_MAKE_LITERAL_T(char, "test char\n");
		opt.print(test);
		EXPECT_EQ(last_printed_string, std::string{ test });
	}
	{
		fea::get_opt<wchar_t> opt{ print_to_wstring };
		const wchar_t* test = FEA_MAKE_LITERAL_T(wchar_t, "test wchar\n");
		opt.print(test);
		EXPECT_EQ(last_printed_wstring, std::wstring{ test });
	}
	{
		fea::get_opt<char16_t> opt{ print_to_string16 };
		const char16_t* test = FEA_MAKE_LITERAL_T(char16_t, "test char16\n");
		std::string utf8 = fea::utf16_to_utf8({ test });
		opt.print(test);
		EXPECT_EQ(last_printed_string, utf8);
	}
	{
		fea::get_opt<char32_t> opt{ print_to_string32 };
		const char32_t* test = FEA_MAKE_LITERAL_T(char32_t, "test char32\n");
		std::string utf8 = fea::utf32_to_utf8({ test });
		opt.print(test);
		EXPECT_EQ(last_printed_string, utf8);
	}
}

TEST(getopt, basics) {


	{
		using mchar_t = char;
		fea::get_opt<mchar_t> opt{ print_to_string };
		// fea::get_opt<char> opt{};
		add_options(opt);

		{
			test_scenario<mchar_t> scenario = test_all_help<mchar_t>();
			scenario.fuzzit(opt);
		}
		{
			test_scenario<mchar_t> scenario = test_2raw<mchar_t>();
			scenario.fuzzit(opt);
		}
	}

	{
		using mchar_t = wchar_t;
		fea::get_opt<mchar_t> opt{ print_to_wstring };
		// fea::get_opt<wchar_t> opt{};
		add_options(opt);

		{
			test_scenario<mchar_t> scenario = test_all_help<mchar_t>();
			scenario.fuzzit(opt);
		}
		{
			test_scenario<mchar_t> scenario = test_2raw<mchar_t>();
			scenario.fuzzit(opt);
		}
	}

	{
		using mchar_t = char16_t;
		fea::get_opt<mchar_t> opt{ print_to_string16 };
		// fea::get_opt<char16_t> opt{};
		add_options(opt);

		{
			test_scenario<mchar_t> scenario = test_all_help<mchar_t>();
			scenario.fuzzit(opt);
		}
		{
			test_scenario<mchar_t> scenario = test_2raw<mchar_t>();
			scenario.fuzzit(opt);
		}
	}

	{
		using mchar_t = char32_t;
		fea::get_opt<mchar_t> opt{ print_to_string32 };
		// fea::get_opt<char32_t> opt{};
		add_options(opt);

		{
			test_scenario<mchar_t> scenario = test_all_help<mchar_t>();
			scenario.fuzzit(opt);
		}
		{
			test_scenario<mchar_t> scenario = test_2raw<mchar_t>();
			scenario.fuzzit(opt);
		}
	}
}


} // namespace

int main(int argc, char** argv) {
	exe_path = fea::executable_dir(argv[0]);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
