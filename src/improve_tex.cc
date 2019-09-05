#include "improve_tex.h"
#include "string.h"
#include "utilities.h"

#include <iostream>
#include <regex>

using std::regex;
using std::regex_replace;
using std::reverse;
using std::string;
using std::vector;

static string space_digits_into_3digit_groups(const string& tex) {
	string res;
	auto parse = [&](size_t beg, size_t end) {
		if (end - beg < 4) {
			res.append(tex, beg, end - beg);
			return;
		}

		char left_bound = (beg == 0 ? ' ' : tex[beg - 1]);
		char right_bound = (end == tex.size() ? ' ' : tex[end]);

		// Decide from where to start grouping: right or left
		int pos_mod_to_add_space_before;
		if (right_bound == '.' or left_bound != '.') {
			pos_mod_to_add_space_before = end % 3; // right
		} else if (end - beg < 5) {
			res.append(tex, beg, end - beg); // none
			return;
		} else {
			pos_mod_to_add_space_before = beg % 3; // left
		}

		res += tex[beg];
		while (++beg < end) {
			if (beg % 3 == pos_mod_to_add_space_before)
				res += "\\,";
			res += tex[beg];
		}
	};

	size_t beg = 0;
	for (size_t i = 0; i < tex.size(); ++i) {
		if (isdigit(tex[i]))
			continue;

		if (beg < i)
			parse(beg, i);

		parse(i, i + 1);
		beg = i + 1;
	}

	if (beg < tex.size())
		parse(beg, tex.size());

	return res;
}

static string separate_indexes_from_symbols(const string& tex) {
	return regex_replace(tex, regex(R"#((\{\})?(\^|_))#"), " $2");
}

namespace {

constexpr bool debug = false;

class LatexParser {
	string tex_;
	vector<int> matching_bracket_pos_;

public:
	LatexParser(const string& tex)
	   : tex_(separate_indexes_from_symbols(tex)),
	     matching_bracket_pos_(tex_.size(), -1) {
		fill_matching_bracket_pos();

		if constexpr (debug)
			std::cerr << tex_ << '\n';
	}

	LatexParser(const LatexParser&) = delete;
	LatexParser& operator=(const LatexParser&) = delete;

private:
	void fill_matching_bracket_pos() {
		vector<int> pos_stack;
		for (size_t i = 0; i < tex_.size(); ++i) {
			if (tex_[i] == '{') {
				pos_stack.emplace_back(i);
			} else if (tex_[i] == '}') {
				int j = pos_stack.back();
				pos_stack.pop_back();
				matching_bracket_pos_[i] = j;
				matching_bracket_pos_[j] = i;
			}
		}
	}

public:
	string parse() { return parse(0, tex_.size(), true); }

private:
	struct Symbol {
		string symbol;
		vector<string> arguments;
		string top_index;
		string bottom_index;

		bool has_index() const noexcept {
			return (not top_index.empty() or not bottom_index.empty());
		}

		string to_tex() const {
			string res = symbol;
			if (res.empty() and has_index())
				res = "{}";

			for (const string& arg : arguments)
				res.append({'{'}).append(arg).append({'}'});

			auto append_index = [&res](const string& str) {
				if (str.size() == 1) {
					res += str;
					return;
				}

				res.append({'{'}).append(str).append({'}'});
			};

			if (not bottom_index.empty()) {
				res += '_';
				append_index(bottom_index);
			}

			if (not top_index.empty()) {
				res += '^';
				append_index(top_index);
			}

			return res;
		}
	};

	// Parses range [beg, end)
	string parse(int beg, int end, bool ignore_blanks) {
		int pos = beg;

		auto peek_char = [&] { return (pos == end ? ' ' : tex_[pos]); };

		auto extract_char = [&] { return (pos == end ? ' ' : tex_[pos++]); };

		auto parse_symbol = [&]() -> Symbol {
			char c = extract_char();
			if (c == '{') {
				int mbp = matching_bracket_pos_[pos - 1];
				string res = parse(pos, mbp, true);
				pos = mbp + 1;
				return {res, {}, "", ""};
			}

			if (c == '\\') {
				c = extract_char();
				if (not isalpha(c))
					return {{'\\', c}, {}, "", ""};

				Symbol sym = {{'\\', c}, {}, "", ""};
				while (isalpha(peek_char()))
					sym.symbol += extract_char();

				bool ignore_blanks_inside = not is_one_of(
				   sym.symbol, "\\textrm", "\\mathbf", "\\texttt");

				// Command arguments
				while (peek_char() == '{') {
					int mbp = matching_bracket_pos_[pos];
					string arg = parse(pos + 1, mbp, ignore_blanks_inside);
					pos = mbp + 1;
					sym.arguments.emplace_back(arg);
				}

				return sym;
			}

			return {string(1, c), {}, "", ""};
		};

		// Parse into symbols
		int top_index_symbols = 0;
		int bottom_index_symbols = 0;
		vector<Symbol> symbols;

		auto end_of_symbol = [&] {
			// Two or more symbols in index requires re-parsing e.g. a_1 {}_0,
			// gives a_{1 0}, which after re-parsing gives a_{10}

			if (top_index_symbols > 1) {
				auto& idx = symbols.back().top_index;
				idx = LatexParser(idx).parse();
			}

			if (bottom_index_symbols > 1) {
				auto& idx = symbols.back().bottom_index;
				idx = LatexParser(idx).parse();
			}

			// Merge the same commands
			if (symbols.size() > 1) {
				auto& prev_symbol = symbols.end()[-2];
				auto& curr_symbol = symbols.back();
				if (not prev_symbol.has_index() and
				    prev_symbol.symbol == curr_symbol.symbol and
				    prev_symbol.arguments.size() == 1 and
				    curr_symbol.arguments.size() == 1) {
					prev_symbol.arguments[0] += curr_symbol.arguments[0];
					prev_symbol.top_index = curr_symbol.top_index;
					prev_symbol.bottom_index = curr_symbol.bottom_index;
					symbols.pop_back();
				}
			}

			top_index_symbols = 0;
			bottom_index_symbols = 0;
		};

		while (pos < end) {
			if (ignore_blanks and isspace(peek_char())) {
				extract_char();
				continue;
			}

			// Group indexes together
			char c = peek_char();
			if (c == '_' or c == '^') {
				extract_char();
				if (symbols.empty())
					symbols.emplace_back();

				bool bottom_index = (c == '_');
				string& index_tex = (bottom_index ? symbols.back().bottom_index
				                                  : symbols.back().top_index);
				int& index_symbols =
				   (bottom_index ? bottom_index_symbols : top_index_symbols);

				if (not index_tex.empty())
					index_tex += ' ';

				index_tex += parse_symbol().to_tex();
				++index_symbols;
				continue;
			}

			end_of_symbol();
			symbols.emplace_back(parse_symbol());
		}

		end_of_symbol();

		if constexpr (debug) {
			std::cerr << "[" << beg << ", " << end << ") + " << ignore_blanks
			          << ": '" << tex_.substr(beg, end - beg) << "'\n";
			for (auto& symbol : symbols)
				std::cerr << '\'' << symbol.to_tex() << "'\n";
		}

		return convert_symbols_to_string(symbols);
	}

	string convert_symbols_to_string(const vector<Symbol>& symbols) {
		// Combine symbols, but with removing spaces between some symbols
		auto remove_between_alnum = [last_symbol_is_alnum =
		                                false](const Symbol& s) mutable {
			bool is_alnum = (s.symbol.size() == 1 and isalnum(s.symbol[0]));
			bool res = (is_alnum and last_symbol_is_alnum);
			last_symbol_is_alnum = (is_alnum and not s.has_index());
			return res;
		};

		auto remove_between_colon_and_equation_mark =
		   [last_symbol_is_colon = false](const Symbol& s) mutable {
			   bool res = (last_symbol_is_colon and has_prefix(s.symbol, "="));
			   last_symbol_is_colon = (s.symbol == ":" and not s.has_index());
			   return res;
		   };

		auto remove_before = [](const Symbol& s) {
			return (is_one_of(s.symbol, ",", ".", "'", "`") or
			        has_one_of_prefixes(s.symbol, ")", "]", "!", ";"));
		};

		auto remove_before_symbol_after_left_parenthesis =
		   [last_symbol_is_left_paren = false](const Symbol& s) mutable {
			   bool res = last_symbol_is_left_paren;
			   bool is_left_parenthesis = is_one_of(s.symbol, "(", "[");
			   last_symbol_is_left_paren =
			      (is_left_parenthesis and not s.has_index());
			   return res;
		   };

		auto remove_between_quotation_mark =
		   [last_symbol_is_quotation_mark = false](const Symbol& s) mutable {
			   bool is_quotation_mark = (s.symbol == "\"");
			   bool res = (last_symbol_is_quotation_mark and is_quotation_mark);
			   last_symbol_is_quotation_mark = is_quotation_mark;
			   return res;
		   };

		auto remove_before_quotation_mark_after_alnum =
		   [last_symbol_is_alnum = false](const Symbol& s) mutable {
			   bool res = (last_symbol_is_alnum and s.symbol == "\"");
			   last_symbol_is_alnum =
			      all_of(s.symbol.begin(), s.symbol.end(), isalnum);
			   return res;
		   };

		auto remove_before_alnum_after_quotation_mark =
		   [last_symbol_is_quotation_mark = false](const Symbol& s) mutable {
			   bool is_alnum =
			      all_of(s.symbol.begin(), s.symbol.end(), isalnum);
			   bool res = (last_symbol_is_quotation_mark and is_alnum);
			   last_symbol_is_quotation_mark = (s.symbol == "\"");
			   return res;
		   };

		auto remove_left_parenthesis_after_alnum = [last_symbol_is_alnum =
		                                               false](
		                                              const Symbol& s) mutable {
			bool is_alnum = all_of(s.symbol.begin(), s.symbol.end(), isalnum);
			bool res = (last_symbol_is_alnum and is_one_of(s.symbol, "(", "["));
			last_symbol_is_alnum = (is_alnum and not s.has_index());
			return res;
		};

		auto remove_before_digit_after_floating_point =
		   [seq_digit = false,
		    seq_digit_point = false](const Symbol& s) mutable {
			   bool begins_with_digit = isdigit(s.symbol.front());
			   bool ends_with_digit =
			      (isdigit(s.symbol.back()) and not s.has_index());
			   bool is_point = (s.symbol == "." and not s.has_index());

			   bool res = (seq_digit_point and begins_with_digit);
			   seq_digit_point = (seq_digit and is_point);
			   seq_digit = ends_with_digit;
			   return res;
		   };

		string res;
		for (Symbol const& symbol : symbols) {
			// Need to use | instead of || because all functions have to be
			// called as they update their state for each symbol
			bool remove_last_space =
			   (remove_between_alnum(symbol) |
			    remove_between_colon_and_equation_mark(symbol) |
			    remove_before(symbol) | remove_between_quotation_mark(symbol) |
			    remove_before_quotation_mark_after_alnum(symbol) |
			    remove_before_alnum_after_quotation_mark(symbol) |
			    remove_before_symbol_after_left_parenthesis(symbol) |
			    remove_left_parenthesis_after_alnum(symbol) |
			    remove_before_digit_after_floating_point(symbol));
			if (remove_last_space and not res.empty())
				res.pop_back();

			res += symbol.to_tex();
			res += ' ';
		}

		if (not res.empty())
			res.pop_back(); // Remove trailing space

		return res;
	}
};

} // namespace

string improve_tex(const string& tex) {
	return space_digits_into_3digit_groups(LatexParser(tex).parse());
}
