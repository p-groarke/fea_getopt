/*
BSD 3-Clause License

Copyright (c) 2020, Philippe Groarke
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#include "ns_getopt.h" // temp, removeme

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <fea_state_machines/fsm.hpp>
#include <fea_utils/string.hpp>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>


#define FEA_MAKE_LITERAL_T(CharType, str) \
	[]() { \
		if constexpr (std::is_same_v<CharType, char>) { \
			return str; \
		} else if constexpr (std::is_same_v<CharType, wchar_t>) { \
			return L##str; \
		} else if constexpr (std::is_same_v<CharType, char16_t>) { \
			return u##str; \
		} else if constexpr (std::is_same_v<CharType, char32_t>) { \
			return U##str; \
		} \
	}()

#define FEA_MAKE_LITERAL(str) FEA_MAKE_LITERAL_T(CharT, str)

#define FEA_ML(str) FEA_MAKE_LITERAL_T(CharT, str)

#define FEA_MAKE_CHAR_LITERAL_T(CharType, str) \
	[]() { \
		if constexpr (std::is_same_v<CharType, char>) { \
			return str; \
		} else if constexpr (std::is_same_v<CharType, wchar_t>) { \
			return L##str; \
		} else if constexpr (std::is_same_v<CharType, char16_t>) { \
			return u##str; \
		} else if constexpr (std::is_same_v<CharType, char32_t>) { \
			return U##str; \
		} \
	}()

#define FEA_MAKE_CHAR_LITERAL(str) FEA_MAKE_CHAR_LITERAL_T(CharT, str)

#define FEA_CH(ch) FEA_MAKE_CHAR_LITERAL_T(CharT, ch)


namespace fea {
namespace detail {
template <class T>
struct is_string : std::false_type {};
template <class T>
struct is_string<std::basic_string<T, std::char_traits<T>, std::allocator<T>>>
		: std::true_type {};

template <class T>
inline constexpr bool is_string_v = is_string<T>::value;

template <class T>
struct is_char16_array : std::false_type {};
template <>
struct is_char16_array<char16_t[]> : std::true_type {};
template <size_t N>
struct is_char16_array<char16_t[N]> : std::true_type {};

template <class T>
inline constexpr bool is_char16_array_v = is_char16_array<T>::value;

template <class T>
struct is_char32_array : std::false_type {};
template <>
struct is_char32_array<char32_t[]> : std::true_type {};
template <size_t N>
struct is_char32_array<char32_t[N]> : std::true_type {};

template <class T>
inline constexpr bool is_char32_array_v = is_char32_array<T>::value;

template <class T>
constexpr auto to_utf8_str(const T& t) {
	using char16_str_t = std::basic_string<char16_t, std::char_traits<char16_t>,
			std::allocator<char16_t>>;

	using char32_str_t = std::basic_string<char32_t, std::char_traits<char32_t>,
			std::allocator<char32_t>>;

	if constexpr (std::is_same_v<T, const char16_t*>) {
		return utf16_to_utf8({ t });
	} else if constexpr (is_char16_array_v<T>) {
		return utf16_to_utf8({ t });
	} else if constexpr (std::is_same_v<T, char16_str_t>) {
		return utf16_to_utf8(t);
	} else if constexpr (std::is_same_v<T, const char32_t*>) {
		return utf32_to_utf8({ t });
	} else if constexpr (is_char32_array_v<T>) {
		return utf32_to_utf8({ t });
	} else if constexpr (std::is_same_v<T, char32_str_t>) {
		return utf32_to_utf8(t);
	} else {
		return t;
	}
}

template <class T>
constexpr auto do_c_str(const T& t) {
	if constexpr (is_string_v<T>) {
		return t.c_str();
	} else {
		return t;
	}
}

inline int u16printf(const char16_t* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	auto utf8 = utf16_to_utf8({ fmt });
	int ret = vprintf(utf8.c_str(), args);

	va_end(args);
	return ret;
}

inline int u32printf(const char32_t* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	auto utf8 = utf32_to_utf8({ fmt });
	int ret = vprintf(utf8.c_str(), args);

	va_end(args);
	return ret;
}

template <class CharT>
constexpr auto get_print() {
	if constexpr (std::is_same_v<CharT, char>) {
		return printf;
	} else if constexpr (std::is_same_v<CharT, wchar_t>) {
		return wprintf;
	} else if constexpr (std::is_same_v<CharT, char16_t>) {
		// We will convert char16_t to utf8, so use printf.
		return u16printf;
	} else if constexpr (std::is_same_v<CharT, char32_t>) {
		// We will convert char32_t to utf8, so use printf.
		return u32printf;
	}
}

template <class CharT, class... Args>
void any_print(const std::basic_string<CharT, std::char_traits<CharT>,
					   std::allocator<CharT>>& format,
		const Args&... args) {
	any_print(format.c_str(), args...);
}

template <class CharT, class... Args>
void any_print(const CharT* format, const Args&... args) {
	constexpr auto print_func = get_print<CharT>();

	auto utf8_tup = std::apply(
			[](const auto&... margs) {
				return std::make_tuple(detail::to_utf8_str(margs)...);
			},
			std::forward_as_tuple(args...));

	std::apply(
			[&](const auto&... margs) {
				print_func(format, detail::do_c_str(margs)...);
			},
			utf8_tup);
}

template <class CharT>
auto any_to_utf32(const std::basic_string<CharT, std::char_traits<CharT>,
		std::allocator<CharT>>& str) {
	if constexpr (std::is_same_v<CharT, char>) {
		return utf8_to_utf32(str);
	} else if constexpr (std::is_same_v<CharT, wchar_t>) {
		return utf16_to_utf32(str);
	} else if constexpr (std::is_same_v<CharT, char16_t>) {
		return utf16_to_utf32(str);
	} else if constexpr (std::is_same_v<CharT, char32_t>) {
		return std::u32string{ str };
	} else {
		assert(false);
		throw std::runtime_error{ "any_to_utf8 : unsupported string type" };
	}
}

template <class CharT>
std::basic_string<CharT, std::char_traits<CharT>, std::allocator<CharT>>
utf32_to_any(const std::u32string& str) {
	if constexpr (std::is_same_v<CharT, char>) {
		return utf32_to_utf8(str);
	} else if constexpr (std::is_same_v<CharT, wchar_t>) {
		return utf32_to_utf16_w(str);
	} else if constexpr (std::is_same_v<CharT, char16_t>) {
		return utf32_to_utf16(str);
	} else if constexpr (std::is_same_v<CharT, char32_t>) {
		return str;
	} else {
		assert(false);
		throw std::runtime_error{ "any_to_utf8 : unsupported string type" };
	}
}

enum class user_option_e : std::uint8_t {
	flag,
	required_arg,
	optional_arg,
	default_arg,
	multi_arg,
	raw_arg,
	count,
};

template <class CharT = char>
struct user_option {
	using string = std::basic_string<CharT, std::char_traits<CharT>,
			std::allocator<CharT>>;

	user_option(string&& longopt, CharT shortopt, user_option_e t,
			std::function<bool()> func, string&& help)
			: long_opt(longopt)
			, short_opt(shortopt)
			, opt_type(t)
			, flag_func(std::move(func))
			, help_message(std::move(help)) {
	}
	user_option(string&& longopt, CharT shortopt, user_option_e t,
			std::function<bool(string&&)> func, string&& help)
			: long_opt(longopt)
			, short_opt(shortopt)
			, opt_type(t)
			, one_arg_func(std::move(func))
			, help_message(std::move(help)) {
	}
	user_option(string&& longopt, CharT shortopt, user_option_e t,
			std::function<bool(string&&)> func, string&& help,
			string&& default_val)
			: long_opt(longopt)
			, short_opt(shortopt)
			, opt_type(t)
			, one_arg_func(std::move(func))
			, help_message(std::move(help))
			, default_val(std::move(default_val)) {
	}
	user_option(string&& longopt, CharT shortopt, user_option_e t,
			std::function<bool(std::vector<string>&&)> func, string&& help)
			: long_opt(longopt)
			, short_opt(shortopt)
			, opt_type(t)
			, multi_arg_func(std::move(func))
			, help_message(std::move(help)) {
	}


	string long_opt;
	CharT short_opt;
	user_option_e opt_type = user_option_e::count;

	std::function<bool()> flag_func;
	std::function<bool(string&&)> one_arg_func;
	std::function<bool(std::vector<string>&&)> multi_arg_func;

	string help_message;
	string default_val;
};
} // namespace detail


// get_opt supports all char types.
// Uses printf if you provide char.
// Uses wprintf if you provide wchar_t.

// By default, will convert char16_t and char32_t into utf8 and will print
// with printf. You can customize the print function. If it is customized,
// it will be used as-is. Must be a compatible signature with printf.
template <class CharT = char,
		class PrintfT = decltype(detail::get_print<CharT>())>
struct get_opt {
	using string = std::basic_string<CharT, std::char_traits<CharT>,
			std::allocator<CharT>>;

	get_opt(size_t output_width = 120);

	// Construct using a custom print function. The function signature must
	// match printf.
	get_opt(PrintfT printf_func, size_t output_width = 120);

	// An option that uses "raw args". Raw args do not have '--' or '-' in
	// front of them. They are often file names or strings. These will be
	// parsed in the order of appearance. ex : 'my_tool a/raw/arg.txt'
	// Quotes will be added to the name.
	void add_raw_option(string&& help_name,
			std::function<bool(string&&)>&& func, string&& help);

	// An option that doesn't need any argument. AKA a flag.
	// ex : '--flag'
	void add_flag_option(string&& long_arg, std::function<bool()>&& func,
			string&& help, CharT short_arg = FEA_CH('\0'));

	// An option that can accept a single argument or not.
	// If no user argument is provided, your callback is called with your
	// default argument.
	// ex : '--has_default arg' or '--has_default'
	void add_default_arg_option(string&& long_arg,
			std::function<bool(string&&)>&& func, string&& help,
			string&& default_value, CharT short_arg = FEA_CH('\0'));

	// An option that can accept a single argument or not.
	// ex : '--optional arg' or '--optional'
	void add_optional_arg_option(string&& long_arg,
			std::function<bool(string&&)>&& func, string&& help,
			CharT short_arg = FEA_CH('\0'));

	// An option that requires a single argument to be set.
	// ex : '--required arg'
	void add_required_arg_option(string&& long_arg,
			std::function<bool(string&&)>&& func, string&& help,
			CharT short_arg = FEA_CH('\0'));

	// An option that accepts multiple arguments, enclosed in quotes.
	// ex : '--multi "a b c d"'
	void add_multi_arg_option(string&& long_arg,
			std::function<bool(std::vector<string>&&)>&& func, string&& help,
			CharT short_arg = FEA_CH('\0'));

	// Add behavior that requires the first argument (argv[0]).
	// The first argument is always the execution path.
	void add_arg0_behavior(std::function<bool(string&&)>&& func);

	// Adds some text before printing the help.
	void add_help_intro(const string& message);

	// Adds some text after printing the help.
	void add_help_outro(const string& message);

	// Parse the arguments, execute your callbacks, returns success bool
	// (and prints help if there was an error).
	bool parse_options(size_t argc, CharT const* const* argv);

	template <class... Args>
	void print(const string& format, const Args&... args) const;
	template <class... Args>
	void print(const CharT* format, const Args&... args) const;

private:
	static_assert(
			std::is_same_v<CharT,
					char> || std::is_same_v<CharT, wchar_t> || std::is_same_v<CharT, char16_t> || std::is_same_v<CharT, char32_t>,
			"getopt : unknown character type, getopt only supports char, "
			"wchar_t, char16_t and char32_t");

	enum class state {
		arg0,
		choose_parsing,
		parse_longarg,
		parse_shortarg,
		parse_concat,
		parse_raw,
		end,
		count
	};
	enum class transition {
		parse_next,
		exit,
		error,
		help,
		do_longarg,
		do_shortarg,
		do_concat,
		do_raw,
		count
	};

	using fsm_t = typename fsm<transition, state, void(get_opt*)>;
	using state_t = typename fsm_t::state_t;

	std::unique_ptr<fsm_t> make_machine() const;

	void on_arg0_enter(fsm_t&);
	void on_parse_next_enter(fsm_t&);
	void on_parse_longarg(fsm_t&);
	void on_parse_shortarg(fsm_t&);
	void on_parse_concat(fsm_t&);
	void on_parse_raw(fsm_t&);
	void on_print_error(fsm_t&);
	void on_print_help(fsm_t&);

	std::unique_ptr<fsm_t> _machine = make_machine();

	std::unordered_map<CharT, string> _short_opt_to_long_opt;
	std::unordered_map<string, detail::user_option<CharT>>
			_long_opt_to_user_opt;
	std::vector<detail::user_option<CharT>> _raw_user_opts;
	std::function<bool(string&&)> _arg0_func;

	std::vector<string> _args;
	PrintfT _print_func;

	string _help_intro;
	string _help_outro;

	size_t _output_width = 120;

	// State machine eval things :
	std::deque<string> _parsing_args;
	string _error_message;
};

template <class CharT, class PrintfT>
get_opt<CharT, PrintfT>::get_opt(size_t output_width /* = 120*/)
		: _print_func(detail::get_print<CharT>())
		, _output_width(output_width) {
}
template <class CharT, class PrintfT>
get_opt<CharT, PrintfT>::get_opt(
		PrintfT printf_func, size_t output_width /* = 120*/)
		: _print_func(printf_func)
		, _output_width(output_width) {
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_raw_option(string&& help_name,
		std::function<bool(string&&)>&& func, string&& help) {
	using namespace detail;
	_raw_user_opts.push_back(user_option<CharT>{
			std::move(FEA_ML("\"") + help_name + FEA_ML("\"")),
			FEA_CH('\0'),
			user_option_e::raw_arg,
			std::move(func),
			std::move(help),
	});
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_flag_option(string&& long_arg,
		std::function<bool()>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
	using namespace detail;
	_long_opt_to_user_opt.insert({ long_arg,
			user_option<CharT>{
					std::move(long_arg),
					short_arg,
					user_option_e::flag,
					std::move(func),
					std::move(help),
			} });
}
template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_required_arg_option(string&& long_arg,
		std::function<bool(string&&)>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_optional_arg_option(string&& long_arg,
		std::function<bool(string&&)>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_default_arg_option(string&& long_arg,
		std::function<bool(string&&)>&& func, string&& help,
		string&& default_value, CharT short_arg /*= '\0'*/) {
	using namespace detail;
	_long_opt_to_user_opt.insert({ long_arg,
			user_option<CharT>{
					std::move(long_arg),
					short_arg,
					user_option_e::default_arg,
					std::move(func),
					std::move(help),
					std::move(default_value),
			} });
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_multi_arg_option(string&& long_arg,
		std::function<bool(std::vector<string>&&)>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_arg0_behavior(
		std::function<bool(string&&)>&& func) {
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_help_intro(const string& message) {
	_help_intro = message;
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_help_outro(const string& message) {
	_help_outro = message;
}


template <class CharT, class PrintfT>
bool get_opt<CharT, PrintfT>::parse_options(
		size_t argc, CharT const* const* argv) {
	_machine->reset();

	_args.clear();
	_parsing_args.clear();

	_args.reserve(argc);
	//_parsing_args.reserve(argc);

	for (size_t i = 0; i < argc; ++i) {
		_args.push_back({ argv[i] });
		_parsing_args.push_back({ argv[i] });
	}

	while (!_machine->finished()) {
		_machine->update(this);
	}

	return true;
}


template <class CharT, class PrintfT>
template <class... Args>
void get_opt<CharT, PrintfT>::print(
		const string& format, const Args&... args) const {
	print(format.c_str(), args...);
}

template <class CharT, class PrintfT>
template <class... Args>
void get_opt<CharT, PrintfT>::print(
		const CharT* format, const Args&... args) const {

	auto utf8_tup = std::apply(
			[](const auto&... margs) {
				return std::make_tuple(detail::to_utf8_str(margs)...);
			},
			std::forward_as_tuple(args...));

	std::apply(
			[&, this](const auto&... margs) {
				_print_func(format, detail::do_c_str(margs)...);
			},
			utf8_tup);
}


template <class CharT, class PrintfT>
std::unique_ptr<typename get_opt<CharT, PrintfT>::fsm_t>
get_opt<CharT, PrintfT>::make_machine() const {
	std::unique_ptr<fsm_t> ret = std::make_unique<fsm_t>();

	// arg0
	{
		state_t arg0_state;
		arg0_state.add_transition<transition::parse_next,
				state::choose_parsing>();
		arg0_state.add_transition<transition::exit, state::end>();
		arg0_state.add_transition<transition::error, state::end>();

		arg0_state.add_event<fsm_event::on_enter>(&get_opt::on_arg0_enter);
		ret->add_state<state::arg0>(std::move(arg0_state));
	}

	// choose_parsing
	{
		state_t choose_state;
		choose_state
				.add_transition<transition::do_concat, state::parse_concat>();
		choose_state
				.add_transition<transition::do_longarg, state::parse_longarg>();
		choose_state.add_transition<transition::do_shortarg,
				state::parse_shortarg>();
		choose_state.add_transition<transition::do_raw, state::parse_raw>();
		choose_state.add_transition<transition::help, state::end>();
		choose_state.add_transition<transition::exit, state::end>();
		choose_state.add_transition<transition::error, state::end>();

		choose_state.add_event<fsm_event::on_enter>(
				&get_opt::on_parse_next_enter);
		ret->add_state<state::choose_parsing>(std::move(choose_state));
	}

	// end
	{
		state_t end_state;
		end_state.add_transition<transition::help, state::end>();

		end_state.add_event<fsm_event::on_enter_from, transition::error>(
				&get_opt::on_print_error);
		end_state.add_event<fsm_event::on_enter_from, transition::help>(
				&get_opt::on_print_help);

		ret->add_state<state::end>(std::move(end_state));
		ret->set_finish_state<state::end>();
	}

	return ret;
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::on_arg0_enter(fsm_t& m) {
	if (_parsing_args.empty()) {
		return m.trigger<transition::error>(this);
	}

	bool success = true;
	if (_arg0_func) {
		success = std::invoke(_arg0_func, std::move(_parsing_args.front()));
	}

	_parsing_args.pop_front();

	if (!success) {
		return m.trigger<transition::error>(this);
	}

	if (_parsing_args.empty()) {
		return m.trigger<transition::exit>(this);
	}

	return m.trigger<transition::parse_next>(this);
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::on_parse_next_enter(fsm_t& m) {

	string& first = _parsing_args.front();

	// help
	if (first == FEA_ML("-h") || first == FEA_ML("--help")
			|| first == FEA_ML("/?") || first == FEA_ML("/help")) {
		return m.trigger<transition::help>(this);
	}

	// A single short arg, ex : '-d'
	if (fea::starts_with(first, FEA_ML("-")) && first.size() == 2) {
		return m.trigger<transition::do_shortarg>(this);
	}

	// A long arg, ex '--something'
	if (fea::starts_with(first, FEA_ML("--"))) {
		return m.trigger<transition::do_longarg>(this);
	}

	// Concatenated short args, ex '-abdsc'
	if (fea::starts_with(first, FEA_ML("-"))) {
		return m.trigger<transition::do_concat>(this);
	}

	// Everything else failed, check raw args. ex '"some arg"'
	return m.trigger<transition::do_raw>(this);
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::on_print_error(fsm_t& m) {
	print(FEA_ML("problem parsing provided options :\n"));
	print(_error_message);
	print(FEA_ML("\n\n"));
	m.trigger<transition::help>(this);
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::on_print_help(fsm_t&) {
	using namespace detail;

	// The first string is printed as-is.
	// Tries to find '\n'. If it does, splits the incoming string and prints at
	// indentation.
	// If there is no '\n', simply prints str and returns.
	auto print_description = [this](const string& desc, size_t indendation) {
		if (desc.empty())
			return;

		// Split the string if it contains \n, so substrings start on a new line
		// with the appropriate indentation.
		std::vector<string> str_vec;
		if (desc.find(FEA_CH('\n')) == string::npos) {
			// There is no '\n'.
			str_vec.push_back(desc);
		} else {
			// There is '\n'
			str_vec = fea::split(desc, FEA_CH('\n'));
		}

		// Now, make sure all strings are less than output_width in *real* size.

		// To do that, we must convert to utf32 to do our work. This includes
		// getting "language" size, and splitting the strings.

		// If some strings are wider than the max output, split them at the
		// first word possible, so it's pretty. Insert the split strings
		// in the final output vector.
		// If no splitting is required, simply add to the vector.
		std::vector<string> out_str_vec;
		out_str_vec.reserve(out_str_vec.size());

		for (const string& s : str_vec) {
			// Get a utf32 string to work without brain damage.
			std::u32string utf32 = detail::any_to_utf32(s);
			// Gets the actual lexical size of the string. We include the
			// indentation width that comes before.
			size_t real_size = utf32.size() + indendation;

			// We must splitty split.
			if (real_size > _output_width) {
				size_t pos = 0;
				// Find substrings and split them.
				while (real_size - pos > _output_width) {
					std::u32string new_str
							= utf32.substr(pos, _output_width - indendation);
					// Find where we can split nicely.
					size_t space_pos = new_str.find_last_of(U' ');

					// Pushback the string, split nicely at the last word.
					out_str_vec.push_back(detail::utf32_to_any<CharT>(
							new_str.substr(0, space_pos)));

					// Don't forget to ignore the space for the next sentence.
					pos += space_pos + 1;
				}
				// We still have 1 leftover split at the end.
				std::u32string new_str
						= utf32.substr(pos, _output_width - indendation);
				out_str_vec.push_back(detail::utf32_to_any<CharT>(new_str));

			} else {
				out_str_vec.push_back(std::move(s));
			}
		}


		// Finally, print everything at the right indentation.
		for (size_t i = 0; i < out_str_vec.size(); ++i) {
			const string& substr = out_str_vec[i];
			print(FEA_ML("%s\n"), substr.c_str());

			// Print the indentation for the next string if there is one.
			if (i + 1 < out_str_vec.size()) {
				print(FEA_ML("%*s"), int(indendation), FEA_ML(""));
			}
		}
	};

	constexpr size_t indent = 1;
	constexpr size_t shortopt_width = 4;
	constexpr size_t shortopt_total_width = indent + shortopt_width;
	constexpr size_t longopt_space = 2;
	constexpr size_t longopt_width_max = 30;
	constexpr size_t rawopt_help_indent = 4;
	const string opt_str = FEA_ML(" <optional>");
	const string req_str = FEA_ML(" <value>");
	const string multi_str = FEA_ML(" 'mul ti ple'");
	const string default_beg = FEA_ML(" <=");
	const string default_end = FEA_ML(">");

	if (!_help_intro.empty()) {
		print(_help_intro + FEA_ML("\n"));
	}

	// Usage
	{
		string out_str;
		for (const user_option<CharT>& raw_opt : _raw_user_opts) {
			out_str += FEA_ML(" ");
			out_str += raw_opt.long_opt;
		}

		print(FEA_ML("\nUsage: %s%s [options]\n\n"), _args.front().c_str(),
				out_str.c_str());
	}

	// Raw Options
	if (!_raw_user_opts.empty()) {
		// Find the biggest raw option name size.
		// The raw option's name is stored in its long_opt string.
		size_t max_name_width = 0;
		for (const user_option<CharT> raw_opt : _raw_user_opts) {
			size_t name_width = raw_opt.long_opt.size() + rawopt_help_indent;
			max_name_width = std::max(max_name_width, name_width);
		}

		// Print section header.
		print(FEA_ML("Arguments:\n"));

		// Now, print the raw option help.
		for (const user_option<CharT> raw_opt : _raw_user_opts) {
			// Print indentation.
			print(FEA_ML("%*s"), int(indent), FEA_ML(""));

			// Print the help, and use max_name_width so each help line is
			// properly aligned.
			print(FEA_ML("%-*s"), int(max_name_width),
					raw_opt.long_opt.c_str());

			// Print the help message. This will split the message if it is too
			// wide, or if the user used '\n' in his message.
			print_description(raw_opt.help_message, indent + max_name_width);
		}
		print(FEA_ML("\n"));
	}

	// All Other Options
	{
		print(FEA_ML("Options:\n"));

		// First, compute the maximum width of long options.
		size_t longopt_width = 0;
		for (const std::pair<string, user_option<CharT>>& opt_p :
				_long_opt_to_user_opt) {
			const string& long_opt_str = opt_p.first;
			const user_option<CharT>& opt = opt_p.second;

			size_t size = 2 + long_opt_str.size() + longopt_space;
			if (opt.opt_type == user_option_e::optional_arg) {
				size += opt_str.size();
			} else if (opt.opt_type == user_option_e::required_arg) {
				size += req_str.size();
			} else if (opt.opt_type == user_option_e::default_arg) {
				size += default_beg.size() + opt.default_val.size()
						+ default_end.size();
			} else if (opt.opt_type == user_option_e::multi_arg) {
				size += multi_str.size();
			}

			longopt_width = std::max(longopt_width, size);
		}

		// Cap it to longopt_width_max though. If it is bigger than this, we'll
		// print it on a new line.
		if (longopt_width > longopt_width_max) {
			longopt_width = longopt_width_max;
		}

		// Print the options.
		for (const std::pair<string, user_option<CharT>>& opt_p :
				_long_opt_to_user_opt) {
			const string& long_opt_str = opt_p.first;
			const user_option<CharT>& opt = opt_p.second;

			// Print indentation.
			print(FEA_ML("%*s"), int(indent), FEA_ML(""));

			// If the option has a shortarg, print that.
			if (opt.short_opt != FEA_CH('\0')) {
				string shortopt_str;
				shortopt_str += FEA_ML("-");
				shortopt_str += opt.short_opt;
				shortopt_str += FEA_ML(",");
				print(FEA_ML("%-*s"), int(shortopt_width),
						shortopt_str.c_str());
			} else {
				print(FEA_ML("%*s"), int(shortopt_width), FEA_ML(""));
			}

			// Build the longopt string.
			string longopt_str;
			longopt_str += FEA_ML("--");
			longopt_str += long_opt_str;

			// Add the specific "instructions" for each type of arg.
			if (opt.opt_type == user_option_e::optional_arg) {
				longopt_str += opt_str;
			} else if (opt.opt_type == user_option_e::required_arg) {
				longopt_str += req_str;
			} else if (opt.opt_type == user_option_e::default_arg) {
				longopt_str += default_beg;
				longopt_str += opt.default_val;
				longopt_str += default_end;
			} else if (opt.opt_type == user_option_e::multi_arg) {
				longopt_str += multi_str;
			}

			// Print the longopt string.
			print(FEA_ML("%-*s"), int(longopt_width), longopt_str.c_str());

			// If it was bigger than the max width, the description will be
			// printed on the next line, indented up to the right position.
			if (longopt_str.size() >= longopt_width) {
				print(FEA_ML("\n"));
				print(FEA_ML("%*s"), int(longopt_width + shortopt_total_width),
						FEA_ML(""));
			}

			// Print the help message, indents appropriately and splits into
			// multiple strings if the message is too wide.
			print_description(
					opt.help_message, longopt_width + shortopt_total_width);
		}

		if (longopt_width == 0) // No options, width is --help only.
			longopt_width = 2 + 4 + longopt_space;

		// Print the help command help.
		print(FEA_ML("%*s%-*s%-*s%s\n"), int(indent), FEA_ML(""),
				int(shortopt_width), FEA_ML("-h,"), int(longopt_width),
				FEA_ML("--help"), FEA_ML("Print this help\n"));

		// Print user outro.
		if (!_help_outro.empty()) {
			print(FEA_ML("\n%s\n"), _help_outro.c_str());
		}
	}
} // namespace fea


} // namespace fea
