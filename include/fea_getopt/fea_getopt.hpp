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

template <class CharT = char>
struct user_option {
	enum class type : std::uint8_t {
		no_arg,
		required_arg,
		optional_arg,
		default_arg,
		multi_arg,
		raw_arg,
		count,
	};

	using string = std::basic_string<CharT, std::char_traits<CharT>,
			std::allocator<CharT>>;

	string long_arg;

	std::function<bool()> flag_func;
	std::function<bool(string&& arg)> one_arg_func;
	std::function<bool(std::vector<string>&& args)> multi_arg_func;

	type _type = type::count;
	CharT short_arg;
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

	get_opt();

	// Construct using a custom print function. The function signature must
	// match printf.
	get_opt(PrintfT printf_func);

	// An option that uses "raw args". Raw args do not have '--' or '-' in
	// front of them. They are often file names or strings. These will be
	// parsed in the order of appearance. ex : 'my_tool a/raw/arg.txt'
	void add_raw_arg(string&& help_name, std::function<bool(string&&)>&& func,
			string&& help);

	// An option that doesn't need any argument. AKA a flag.
	// ex : '--flag'
	void add_flag(string&& long_arg, std::function<bool()>&& func,
			string&& help, CharT short_arg = '\0');

	// An option that can accept a single argument or not.
	// If no user argument is provided, your callback is called with your
	// default argument.
	// ex : '--has_default arg' or '--has_default'
	void add_default_arg(string&& long_arg,
			std::function<bool(string&&)>&& func, string&& help,
			string&& default_value, CharT short_arg = '\0');

	// An option that can accept a single argument or not.
	// ex : '--optional arg' or '--optional'
	void add_optional_arg(string&& long_arg,
			std::function<bool(string&&)>&& func, string&& help,
			CharT short_arg = '\0');

	// An option that requires a single argument to be set.
	// ex : '--required arg'
	void add_required_arg(string&& long_arg,
			std::function<bool(string&&)>&& func, string&& help,
			CharT short_arg = '\0');

	// An option that accepts multiple arguments, enclosed in quotes.
	// ex : '--multi "a b c d"'
	void add_multi_arg(string&& long_arg,
			std::function<bool(std::vector<string>&&)>&& func, string&& help,
			CharT short_arg = '\0');

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

	void print_help() const;

	template <class... Args>
	void print(const string& format, const Args&... args) const;
	template <class... Args>
	void print(const CharT* format, const Args&... args) const;

private:
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

	PrintfT _print_func;

	string _help_intro;
	string _help_outro;

	std::vector<string> _args;
	std::deque<string> _parsing_args;
	string _error_message;
};

template <class CharT, class PrintfT>
get_opt<CharT, PrintfT>::get_opt()
		: _print_func(detail::get_print<CharT>()) {
}
template <class CharT, class PrintfT>
get_opt<CharT, PrintfT>::get_opt(PrintfT printf_func)
		: _print_func(printf_func) {
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_raw_arg(string&& help_name,
		std::function<bool(string&&)>&& func, string&& help) {
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_flag(string&& long_arg,
		std::function<bool()>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
}
template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_required_arg(string&& long_arg,
		std::function<bool(string&&)>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_optional_arg(string&& long_arg,
		std::function<bool(string&&)>&& func, string&& help,
		CharT short_arg /*= '\0'*/) {
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_default_arg(string&& long_arg,
		std::function<bool(string&&)>&& func, string&& help,
		string&& default_value, CharT short_arg /*= '\0'*/) {
}

template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::add_multi_arg(string&& long_arg,
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

	_args.reserve(argc);
	_parsing_args.reserve(argc);

	for (size_t i = 0; i < argc; ++i) {
		_args.push_back({ argv[i] });
		_parsing_args.push_back({ argv[i] });
	}

	while (!_machine.finished()) {
		_machine.update(*this);
	}

	return true;
}


template <class CharT, class PrintfT>
void get_opt<CharT, PrintfT>::print_help() const {
	using namespace detail;

	const size_t first_space = 1;
	const size_t shortarg_width = 4;
	const size_t shortarg_total_width = first_space + shortarg_width;
	const size_t longarg_space = 2;
	const size_t longarg_width_max = 30;
	const size_t rawarg_space = 4;
	const string opt_str = FEA_ML(" <optional>");
	const string req_str = FEA_ML(" <value>");
	const string multi_str = FEA_ML(" 'mul ti ple'");
	const string default_beg = FEA_ML(" <=");
	const string default_end = FEA_ML(">");

	print(_help_intro + FEA_ML("\n"));

	{ /* Usage. */
		string out_str;
		for (const user_option& raw_opt : _raw_user_opts) {
			out_str += FEA_ML(" ");
			out_str += raw_opt->long_arg;
		}

		print(FEA_ML("\nUsage: %s%s [options]\n\n"), _args.front().c_str(),
				out_str);
	}

	//{ /* Raw args. */
	//	bool has_raw_args = false;
	//	size_t name_width = 0;
	//	for (const argument* x = args; x < args + args_size; x++) {
	//		if (x->arg_type != type::raw_arg)
	//			continue;

	//		has_raw_args = true;
	//		size_t s = x->long_arg.size() + rawarg_space;
	//		if (s > name_width)
	//			name_width = s;
	//	}

	//	if (has_raw_args)
	//		printf("Arguments:\n");

	//	for (const argument* x = args; x < args + args_size; x++) {
	//		if (x->arg_type != type::raw_arg)
	//			continue;
	//		printf("%*s", (int)first_space, "");
	//		printf("%-*.*s", (int)name_width, (int)x->long_arg.size(),
	//				x->long_arg.data());
	//		print_description(x->description, first_space + name_width);
	//	}
	//	if (has_raw_args)
	//		printf("\n");
	//}

	//{ /* Other args.*/
	//	printf("Options:\n");
	//	size_t la_width = 0;
	//	for (const argument* x = args; x < args + args_size; x++) {
	//		if (x->arg_type == type::raw_arg)
	//			continue;

	//		size_t s = 2 + x->long_arg.size() + longarg_space;
	//		if (x->arg_type == type::optional_arg) {
	//			s += opt_str.size();
	//		} else if (x->arg_type == type::required_arg) {
	//			s += req_str.size();
	//		} else if (x->arg_type == type::default_arg) {
	//			s += default_beg.size() + x->default_arg.size()
	//					+ default_end.size();
	//		} else if (x->arg_type == type::multi_arg) {
	//			s += multi_str.size();
	//		}

	//		if (s > la_width)
	//			la_width = s;
	//	}

	//	if (la_width > longarg_width_max) {
	//		la_width = longarg_width_max;
	//	}

	//	for (const argument* x = args; x < args + args_size; x++) {
	//		if (x->arg_type == type::raw_arg)
	//			continue;

	//		printf("%*s", (int)first_space, "");

	//		if (x->short_arg != '\0') {
	//			stack_string s_arg;
	//			s_arg += "-";
	//			s_arg += x->short_arg;
	//			s_arg += ",";
	//			printf("%-*s", (int)shortarg_width, s_arg.c_str());
	//		} else {
	//			printf("%*s", (int)shortarg_width, "");
	//		}

	//		stack_string la_str;
	//		la_str += "--";
	//		la_str += x->long_arg;
	//		if (x->arg_type == type::optional_arg) {
	//			la_str += opt_str;
	//		} else if (x->arg_type == type::required_arg) {
	//			la_str += req_str;
	//		} else if (x->arg_type == type::default_arg) {
	//			la_str += default_beg;
	//			la_str += x->default_arg;
	//			la_str += default_end;
	//		} else if (x->arg_type == type::multi_arg) {
	//			la_str += multi_str;
	//		}

	//		printf("%-*s", (int)la_width, la_str.c_str());

	//		// TODO: Verify comparison is still ok (size works as exapected).
	//		if (la_str.size() >= la_width) {
	//			printf("\n");
	//			printf("%*s", (int)(la_width + shortarg_total_width), "");
	//		}

	//		print_description(x->description, la_width + shortarg_total_width);
	//	}

	//	if (la_width == 0) // No options, width is --help only.
	//		la_width = 2 + 4 + longarg_space;

	//	printf("%*s%-*s%-*s%s\n", (int)first_space, "", (int)shortarg_width,
	//			"-h,", (int)la_width, "--help", "Print this help\n");

	//	printf("\n%.*s\n", (int)option.help_outro.size(),
	//			option.help_outro.data());
	//}
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
	// print(FEA_MAKE_LITERAL(problem parsing arguments, aborting\n));
	// m.trigger<transition::help>(me, args);
}


} // namespace fea
