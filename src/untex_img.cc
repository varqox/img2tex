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
using std::variant;
using std::vector;

namespace {

constexpr bool debug = false;

class ImgUntexer {
	static constexpr int SYMBOL_GROUPS_NO = 7;
	static constexpr double MATCH_THRESHOLD = 1.5;
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
	ImgUntexer(Matrix<int> image, const SymbolDatabase& symbol_database,
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

				verbose_log(pos, ": ", opt->last_symbol.matched_symbol_tex,
				            " with cum_diff: ", fixed, setprecision(6),
				            opt->best_cumulative_diff, '\n');
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
			verbose_log("\033[1;32mMatched as group ", symbol_group, ":\033[m ",
			            best_symbol_tex, " with diff: ", setprecision(6), fixed,
			            best_diff, '\n');
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
			verbose_log("\033[33mBest match as group ", symbol_group,
			            ":\033[m ", best_symbol_tex,
			            " with diff: ", setprecision(6), fixed, best_diff,
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
};

} // namespace

variant<string, UntexFailure> untex_img(const Matrix<int>& img,
                                        const SymbolDatabase& symbol_database,
                                        bool be_verbose) {
	return ImgUntexer(img, symbol_database, be_verbose).untex();
}
