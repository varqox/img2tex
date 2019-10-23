#include "untex_img.h"
#include "improve_tex.h"
#include "symbol_database.h"
#include "utilities.h"

using std::array;
using std::fixed;
using std::max;
using std::min;
using std::nullopt;
using std::numeric_limits;
using std::optional;
using std::setprecision;
using std::string;
using std::string_view;
using std::variant;
using std::vector;

namespace {

constexpr bool debug = false;

class ImgUntexer {
	static constexpr int SYMBOL_GROUPS_NO = 12;
	static constexpr double MATCH_THRESHOLD = 1.4;
	static constexpr int SIZE_DIFF_THRESHOLD = 4;

	struct MatchedSymbol {
		int orig_symbol_group;
		const SplitSymbol& orig_symbol;
		string matched_symbol_tex;
	};

	struct PossibleDpState {
		double best_cumulative_diff = numeric_limits<double>::max();
		MatchedSymbol last_symbol;
	};

	Matrix<int> orignal_image_;
	const SymbolDatabase& symbols_db_;
	bool be_verbose_;
	array<vector<SplitSymbol>, SYMBOL_GROUPS_NO> symbol_groups_;
	vector<optional<PossibleDpState>> dp_;

	template <class... Args>
	void verbose_log(Args&&... args) {
		if (be_verbose_)
			(std::cerr << ... << std::forward<Args>(args));
	}

public:
	ImgUntexer(Matrix<int> image,
	           const SymbolDatabase& symbol_database,
	           bool be_verbose = false)
	   : orignal_image_(std::move(image)), symbols_db_(symbol_database),
	     be_verbose_(be_verbose) {}

private:
	void split_into_symbol_groups() {
		symbol_groups_ =
		   ::split_into_symbol_groups<SYMBOL_GROUPS_NO>(orignal_image_);

		if constexpr (debug) {
			show_matrix(orignal_image_);
			binshow_matrix(orignal_image_);
			for (size_t i = 0; i < symbol_groups_.size(); ++i) {
				verbose_log("symbol_groups_[", i, "]:\n");
				for (auto&& symbol : symbol_groups_[i])
					binshow_matrix(symbol.img);
			}
		}
	}

	bool dp_possible(int pos) const noexcept { return dp_[pos].has_value(); }

	bool cannot_match(int pos) const noexcept {
		if (dp_possible(pos))
			return false;

		const int n = (int)dp_.size();
		if (pos == n - 1)
			return true; // We have to match on the last symbol

		int beg = pos - symbol_groups_.size() + 1;
		if (beg < 0)
			return false; // A symbol from greater group may match later

		for (int i = beg; i < pos; ++i) {
			if (dp_possible(i))
				return false;
		}

		return true;
	}

	variant<vector<MatchedSymbol>, UntexFailure> match_symbols() {
		const int n = symbol_groups_[0].size();
		if (n == 0)
			return vector<MatchedSymbol> {};

		dp_.clear();
		dp_.resize(n);

		for (int pos = 0; pos < n; ++pos) {
			verbose_log("\nSYMBOL No. ", pos, ":\n");
			for (int gr = 0; gr < min<int>(pos + 1, symbol_groups_.size());
			     ++gr) {
				dp_try_to_match_symbol(pos, gr);
			}

			if (cannot_match(pos)) {
				verbose_log('\n');
				return collect_unmatched_symbol_candidates(pos);
			}
		}

		return dp_collect_only_used_symbols();
	}

	vector<MatchedSymbol> dp_collect_only_used_symbols() {
		if constexpr (debug) {
			int pos = -1;
			for (auto& opt : dp_) {
				++pos;
				if (not opt.has_value())
					continue;

				verbose_log(pos,
				            ": ",
				            opt->last_symbol.matched_symbol_tex,
				            " with cum_diff: ",
				            fixed,
				            setprecision(6),
				            opt->best_cumulative_diff,
				            '\n');
			}
		}

		vector<int> valid_positions;
		int pos = dp_.size() - 1;
		while (pos >= 0) {
			valid_positions.emplace_back(pos);
			pos -= 1 + dp_[pos].value().last_symbol.orig_symbol_group;
		}

		// Collect symbols
		vector<MatchedSymbol> symbols;
		reverse(valid_positions.begin(), valid_positions.end());
		for (int valid_pos : valid_positions)
			symbols.emplace_back(dp_[valid_pos].value().last_symbol);

		return symbols;
	}

	void dp_try_to_match_symbol(int pos, int symbol_group) {
		if (pos > symbol_group and not dp_possible(pos - symbol_group - 1))
			return;

		const SplitSymbol& curr_symbol =
		   symbol_groups_[symbol_group][pos - symbol_group];

		double best_diff = numeric_limits<double>::max();
		const Symbol* best_symbol = nullptr;
		// Find best matching symbol
		for (Symbol const& symbol : symbols_db_.symbols()) {
			if (abs(curr_symbol.img.cols() - symbol.img.cols()) >
			       SIZE_DIFF_THRESHOLD or
			    abs(curr_symbol.img.rows() - symbol.img.rows()) >
			       SIZE_DIFF_THRESHOLD) {
				continue;
			}

			double diff = symbols_db_.statistics().img_diff(
			   curr_symbol.img, symbol.img, min(best_diff, MATCH_THRESHOLD));
			if (diff < best_diff) {
				best_diff = diff;
				best_symbol = &symbol;
			}
		}

		if (not best_symbol)
			return;

		string best_symbol_tex =
		   matched_symbol_to_tex(curr_symbol, *best_symbol);
		if (best_diff <= MATCH_THRESHOLD) {
			verbose_log("\033[1;32mMatched as group ",
			            symbol_group,
			            ":\033[m ",
			            best_symbol_tex,
			            " with diff: ",
			            setprecision(6),
			            fixed,
			            best_diff,
			            '\n');
			if (be_verbose_) {
				binshow_matrix(curr_symbol.img);
				binshow_matrix(best_symbol->img);
			}

			double curr_cum_diff =
			   (pos == symbol_group
			       ? 0
			       : dp_[pos - symbol_group - 1].value().best_cumulative_diff) +
			   best_diff;
			bool overwrite_state =
			   (not dp_possible(pos) or
			    curr_cum_diff <= dp_[pos].value().best_cumulative_diff);
			if (overwrite_state) {
				dp_[pos].emplace(PossibleDpState {
				   curr_cum_diff,
				   {symbol_group, curr_symbol, best_symbol_tex}});
			}

		} else if constexpr (debug) {
			verbose_log("\033[33mBest match as group ",
			            symbol_group,
			            ":\033[m ",
			            best_symbol_tex,
			            " with diff: ",
			            setprecision(6),
			            fixed,
			            best_diff,
			            '\n');
		}
	}

	static string matched_symbol_to_tex(const SplitSymbol& current_symbol,
	                                    const Symbol& matched_symbol) {
		switch (matched_symbol.kind) {
		case SymbolKind::INDEX: {
			auto index = matched_symbol.tex.substr(Symbol::INDEX_PREFIX.size());
			if (current_symbol.top_rows_cut < current_symbol.bottom_rows_cut)
				return "{}^" + index;

			return "{}_" + index;
		}
		case SymbolKind::OTHER: return matched_symbol.tex;
		}

		return "";
	}

	int find_longest_matched_prefix_end(int pos) const noexcept {
		for (int i = pos; i >= 0; --i) {
			if (dp_possible(i))
				return i;
		}

		return -1;
	}

	UntexFailure collect_unmatched_symbol_candidates(int pos) {
		UntexFailure res;

		int longest_matched_prefix_end = find_longest_matched_prefix_end(pos);
		const int n = dp_.size();
		for (int gr = 0; gr < min<int>(pos + 1, symbol_groups_.size()); ++gr) {
			// Find all rightmost unmatched symbol candidates:
			// E.g. with symbol_groups_.size() == 3
			// longest_matched_prefix_end v
			//                    __  __  __ | __  __  __
			// Candidate 1:                    ^--------^
			// Candidate 2:               ^---------^
			// Candidate 3:           ^---------^

			const int rightmost =
			   min(longest_matched_prefix_end + 1, n - 1 - gr);
			const int leftmost = max(0, longest_matched_prefix_end - gr + 1);

			for (int cand_pos = rightmost; cand_pos >= leftmost; --cand_pos) {
				bool is_prefix_preceding_cand_pos_possible =
				   (cand_pos == 0 or dp_possible(cand_pos - 1));
				if (not is_prefix_preceding_cand_pos_possible)
					continue;

				auto const& symbol = symbol_groups_[gr][cand_pos];
				res.unmatched_symbol_candidates.emplace_back(symbol);
			}
		}

		return res;
	}

public:
	variant<string, UntexFailure> untex() {
		split_into_symbol_groups();

		using ResType = variant<string, UntexFailure>;
		return std::visit(
		   overloaded {[](UntexFailure failure) { return ResType(failure); },
		               [&](vector<MatchedSymbol> symbols) {
			               correct_matched_symbols_using_baseline(symbols);
			               adjust_symbols_spacing(symbols);
			               string tex;
			               for (auto const& symbol : symbols) {
				               tex += symbol.matched_symbol_tex;
				               tex += ' ';
			               }

			               if constexpr (debug)
				               verbose_log(tex, '\n');

			               assert(not tex.empty());
			               tex.pop_back(); // Remove trailing space
			               return ResType(improve_tex(tex));
		               }},
		   match_symbols());
	}

private:
	static void
	correct_matched_symbols_using_baseline(vector<MatchedSymbol>& symbols) {
		optional<int> baseline_row = detect_baseline_row(symbols);
		if (not baseline_row.has_value())
			return; // Nothing we can do

		for (auto& symbol : symbols) {
			bool is_baseline_symbol =
			   (symbol.orig_symbol.top_rows_cut > baseline_row.value() - 3);
			auto& tex = symbol.matched_symbol_tex;
			if (is_one_of(tex, ".", "\\cdot"))
				tex = (is_baseline_symbol ? "." : "\\cdot");
			else if (is_one_of(tex, "\\ldots", "\\cdots"))
				tex = (is_baseline_symbol ? "\\ldots" : "\\cdots");
		}
	}

	static optional<int>
	detect_baseline_row(const vector<MatchedSymbol>& symbols) {
		using std::string_view_literals::operator""sv;
		// clang-format off
		constexpr std::array baseline_marking_symbols = {
		   "0"sv, "1"sv, "2"sv, "3"sv, "4"sv, "5"sv, "6"sv, "7"sv, "8"sv, "9"sv,
		   "A"sv, "B"sv, "C"sv, "D"sv, "E"sv, "F"sv, "G"sv, "H"sv, "I"sv, "J"sv,
		   "K"sv, "L"sv, "M"sv, "N"sv, "O"sv, "P"sv, "R"sv, "S"sv, "T"sv, "U"sv,
		   "V"sv, "W"sv, "X"sv, "Y"sv, "Z"sv, "\\Delta"sv, "\\Gamma"sv,
		   "\\Lambda"sv, "\\Omega"sv, "\\Phi"sv, "\\Pi"sv, "\\Psi"sv,
		   "\\Sigma"sv, "\\Theta"sv, "\\Upsilon"sv, "\\Xi"sv, "\\alpha"sv,
		   "\\delta"sv, "\\epsilon"sv, "\\iota"sv, "\\kappa"sv, "\\lambda"sv,
		   "\\nu"sv, "\\omega"sv "\\pi"sv, "\\sigma"sv, "\\tau"sv, "\\theta"sv,
		   "\\upsilon"sv, "\\varepsilon"sv, "\\varpi"sv, "\\vartheta"sv, "a"sv,
		   "b"sv, "c"sv, "d"sv, "e"sv, "h"sv, "i"sv, "k"sv, "l"sv, "m"sv, "n"sv,
		   "o"sv, "r"sv, "s"sv, "t"sv, "u"sv, "v"sv, "w"sv, "x"sv, "z"sv,
		};
		// clang-format on

		for (auto const& matched_symbol : symbols) {
			static_assert(is_sorted(baseline_marking_symbols));
			if (binary_search(baseline_marking_symbols,
			                  matched_symbol.matched_symbol_tex)) {
				auto const& orig_symbol = matched_symbol.orig_symbol;
				return orig_symbol.top_rows_cut + orig_symbol.img.rows() - 1;
			}
		}

		return nullopt;
	}

	static void adjust_symbols_spacing(vector<MatchedSymbol>& symbols) {
		if (symbols.empty())
			return;

		vector<int> spacing_after(symbols.size() - 1);
		for (size_t i = 0; i < spacing_after.size(); ++i) {
			spacing_after[i] = symbol_horizontal_distance(
			   symbols[i].orig_symbol, symbols[i + 1].orig_symbol);
		}

		// Replace symbols based on spacing
		for (size_t i = 0; i < symbols.size(); ++i) {
			int left_spacing = (i > 0 ? spacing_after[i - 1] : 0);
			int right_spacing = (i + 1 < symbols.size() ? spacing_after[i] : 0);
			int min_spacing = min(left_spacing, right_spacing);
			auto& symbol = symbols[i];

			if constexpr (debug) {
				std::cerr << left_spacing << "   " << symbol.matched_symbol_tex
				          << "   " << right_spacing << '\n';
			}

			if (symbol.matched_symbol_tex == "|" and min_spacing > 6)
				symbol.matched_symbol_tex = "\\mid";

			if (symbol.matched_symbol_tex == "\\|" and min_spacing > 6)
				symbol.matched_symbol_tex = "\\parallel";
		}

		// Append detected spacing to the previous symbol
		for (size_t i = 0; i < symbols.size() - 1; ++i) {
			auto& l_sym = symbols[i];
			auto& r_sym = symbols[i + 1];
			int spacing = spacing_after[i];
			int raw_spacing = r_sym.orig_symbol.first_column_pos -
			                  l_sym.orig_symbol.first_column_pos -
			                  l_sym.orig_symbol.img.cols();

			bool is_l_text = false;
			bool is_r_text = false;

			constexpr array commands = {"\\mathbf", "\\textrm", "\\texttt"};
			constexpr array spacing_signs = {"~", " ", " "};
			static_assert(commands.size() == spacing_signs.size());

			bool added_spacing = false;
			for (size_t cmd_idx = 0; cmd_idx < commands.size(); ++cmd_idx) {
				auto command = commands[cmd_idx];
				auto spacing_sign = spacing_signs[cmd_idx];

				bool is_l_bsc_command =
				   is_basic_command(command, l_sym.matched_symbol_tex);
				bool is_r_bsc_command =
				   is_basic_command(command, r_sym.matched_symbol_tex);
				if (spacing > 5 and is_l_bsc_command and is_r_bsc_command) {
					l_sym.matched_symbol_tex.append(command)
					   .append("{")
					   .append(spacing_sign)
					   .append("}");
					added_spacing = true;
					break;
				}

				is_l_text |= is_l_bsc_command;
				is_r_text |= is_r_bsc_command;
			}

			if (added_spacing)
				continue;

			bool l_ends_with_alnum =
			   symbol_ends_with(l_sym.matched_symbol_tex, isalnum);
			bool r_begins_with_alnum =
			   symbol_begins_with(r_sym.matched_symbol_tex, isalnum);

			if ((is_one_of(l_sym.matched_symbol_tex, ")", "!") and is_r_text) or
			    (l_ends_with_alnum and is_r_text) or
			    (is_l_text and r_begins_with_alnum)) {
				if (spacing > 15) {
					l_sym.matched_symbol_tex += " \\quad";
					continue;
				}

				if (spacing > 4) {
					l_sym.matched_symbol_tex += " \\;";
					continue;
				}
			}

			if ((l_ends_with_alnum and
			     has_prefix(r_sym.matched_symbol_tex, "(")) or
			    (has_suffix(l_sym.matched_symbol_tex, ")") and
			     r_begins_with_alnum)) {
				if (spacing > 10) {
					l_sym.matched_symbol_tex += " \\quad";
					continue;
				}

				if (spacing > 6) {
					l_sym.matched_symbol_tex += " \\;";
					continue;
				}
			}

			if (has_suffix(l_sym.matched_symbol_tex, ",") and
			    raw_spacing > 20) {
				l_sym.matched_symbol_tex += "\\quad";
				continue;
			}

			if (has_suffix(l_sym.matched_symbol_tex, ",") and
			    (r_begins_with_alnum or
			     r_sym.matched_symbol_tex == "\\ldots")) {
				if (raw_spacing > 14) {
					l_sym.matched_symbol_tex += " \\quad";
					continue;
				}

				if (raw_spacing > 8) {
					l_sym.matched_symbol_tex += " \\;";
					continue;
				}
			}

			if (has_suffix(l_sym.matched_symbol_tex, ":") or
			    has_prefix(r_sym.matched_symbol_tex, ":")) {
				if (spacing > 20) {
					l_sym.matched_symbol_tex += " \\quad";
					continue;
				}

				if (spacing > 10) {
					l_sym.matched_symbol_tex += " \\;";
					continue;
				}
			}

			if (is_one_of("\\to",
			              l_sym.matched_symbol_tex,
			              r_sym.matched_symbol_tex) and
			    spacing > 20) {
				l_sym.matched_symbol_tex += " \\quad";
				continue;
			}

			bool l_ends_with_digit =
			   symbol_ends_with(l_sym.matched_symbol_tex, isdigit);
			bool r_begins_with_digit =
			   symbol_begins_with(r_sym.matched_symbol_tex, isdigit);
			if (l_ends_with_alnum and r_begins_with_alnum and
			    not(l_ends_with_digit and r_begins_with_digit) and
			    spacing > 6) {
				l_sym.matched_symbol_tex += " \\;";
				continue;
			}
		}
	}

	template <class Func>
	static bool symbol_begins_with(string_view tex, Func&& predicate) {
		if (tex.empty())
			return false;

		if (predicate(tex.front()))
			return true;

		auto begins_with = [&predicate](string_view str) {
			return symbol_begins_with(str, predicate);
		};

		if (is_between(tex, "\\textrm{", "}", begins_with) or
		    is_between(tex, "\\mathbf{", "}", begins_with) or
		    is_between(tex, "\\texttt{", "}", begins_with)) {
			return true;
		}

		return false;
	}

	template <class Func>
	static bool symbol_ends_with(string_view tex, Func&& predicate) {
		if (tex.empty())
			return false;

		if (std::all_of(tex.begin(), tex.end(), predicate))
			return true;

		auto ends_with = [&predicate](string_view str) {
			return symbol_ends_with(str, predicate);
		};

		if (is_between(tex, "\\textrm{", "}", ends_with) or
		    is_between(tex, "\\mathbf{", "}", ends_with) or
		    is_between(tex, "\\texttt{", "}", ends_with) or
		    is_between(tex, "{}_", "", ends_with) or
		    is_between(tex, "{}_{", "}", ends_with) or
		    is_between(tex, "{}^", "", ends_with) or
		    is_between(tex, "{}^{", "}", ends_with)) {
			return true;
		}

		if (predicate(tex[0]) and is_one_of(tex[1], '_', '^'))
			return ends_with(tex.substr(2));

		return false;
	}

	static bool is_basic_command(string_view command, string_view tex) {
		return is_between(
		   tex, string(command) + "{", "}", std::not_fn(contains_braces));
	}

	static bool contains_braces(string_view str) {
		for (char c : str) {
			if (is_one_of(c, '{', '}'))
				return true;
		}

		return false;
	}

	template <class Func>
	static bool is_between(string_view str,
	                       string_view prefix,
	                       string_view suffix,
	                       Func&& predicate) {
		static_assert(std::is_invocable_r_v<bool, Func, string_view>);

		if (not has_prefix(str, prefix) or not has_suffix(str, suffix))
			return false;

		str.remove_prefix(prefix.size());
		str.remove_suffix(suffix.size());

		return predicate(str);
	}
};

} // namespace

variant<string, UntexFailure> untex_img(const Matrix<int>& img,
                                        const SymbolDatabase& symbol_database,
                                        bool be_verbose) {
	return ImgUntexer(img, symbol_database, be_verbose).untex();
}
