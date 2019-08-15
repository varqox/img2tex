#include "commands.h"
#include "symbol_database.h"
#include "symbols_to_tex.h"

#include <filesystem>
#include <unistd.h>

using std::cerr;
using std::cin;
using std::cout;
using std::deque;
using std::endl;
using std::fixed;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::setprecision;
using std::string;
using std::vector;

constexpr const char* GENERATED_SYMBOLS_DB_FILE = "generated_symbols.db";
constexpr const char* MANUAL_SYMBOLS_DB_FILE = "manual_symbols.db";

inline string failed_symbol_file(int group) {
	return "symbol_" + std::to_string(group) + ".txt";
}

int compare_command(int argc, char** argv) {
	if (argc != 2) {
		cerr << "compare commands needs exactly two arguments\n";
		return 1;
	}

	auto fir = teximg_to_matrix(argv[0]);
	auto sec = teximg_to_matrix(argv[1]);

	SymbolDatabase sdb;
	if (access(GENERATED_SYMBOLS_DB_FILE, F_OK) == 0)
		sdb.add_from_file(GENERATED_SYMBOLS_DB_FILE);
	if (access(MANUAL_SYMBOLS_DB_FILE, F_OK) == 0)
		sdb.add_from_file(MANUAL_SYMBOLS_DB_FILE);

	double diff;
	// for (int i = 0; i < 50000; ++i)
	diff = sdb.statistics().img_diff(fir, sec);
	cerr << setprecision(6) << fixed << "\033[32;1m" << diff << "\033[m"
	     << endl;

	return 0;
}

int gen_command(int argc, char**) {
	if (argc > 0) {
		cerr << "gen command takes no arguments\n";
		return 1;
	}

	SymbolDatabase sdb;
	sdb.generate_symbols();
	sdb.save_to_file(GENERATED_SYMBOLS_DB_FILE);
	return 0;
}

int learn_command(int argc, char** argv) {
	if (argc != 1) {
		cerr << "learn command needs an argument\n";
		return 1;
	}

	const char* symbol_file = argv[0];

	string symbol;
	ifstream file(symbol_file);
	if (not file.good())
		throw std::runtime_error("Failed to open specified symbol file");

	for (char c; (c = file.get()), file;)
		symbol += c;

	string tex;
	for (char c; (c = cin.get()), cin;)
		tex += c;

	if (not tex.empty() and tex.back() == '\n')
		tex.pop_back();

	SymbolDatabase sdb;
	if (access(MANUAL_SYMBOLS_DB_FILE, F_OK) == 0)
		sdb.add_from_file(MANUAL_SYMBOLS_DB_FILE);
	sdb.add_symbol_and_append_file(SymbolDatabase::text_to_symbol(symbol), tex,
	                               MANUAL_SYMBOLS_DB_FILE);

	return 0;
}

int tex_command(int argc, char** argv) {
	if (argc != 1) {
		cerr << "tex command needs an argument\n";
		return 1;
	}

	const char* out_file = argv[0];

	string tex;
	char c;
	while ((c = cin.get()), cin)
		tex += c;

	using namespace std::filesystem;

	auto png_file = tex_to_png_file(tex, false);
	if (not copy_file(png_file, out_file, copy_options::overwrite_existing))
		throw std::runtime_error("Failed to copy file");

	return 0;
}

int untex_command(int argc, char** argv) {
	if (argc != 1) {
		cerr << "untex command needs an argument\n";
		return 1;
	}

	const char* png_file = argv[0];

	constexpr bool debug = true;

	if (access(GENERATED_SYMBOLS_DB_FILE, F_OK) != 0) {
		cerr << "generated symbols database does not exist. Run \"gen\" "
		        "command first\n";
		return 1;
	}

	SymbolDatabase sdb;
	if (access(GENERATED_SYMBOLS_DB_FILE, F_OK) == 0)
		sdb.add_from_file(GENERATED_SYMBOLS_DB_FILE);
	if (access(MANUAL_SYMBOLS_DB_FILE, F_OK) == 0)
		sdb.add_from_file(MANUAL_SYMBOLS_DB_FILE);

	if constexpr (debug and false) {
		for (auto const& [symbol, tex] : sdb.symbols()) {
			cerr << tex << '\n';
			binshow_matrix(symbol);
		}

		// sdb.statistics().print();
	}

	auto img = teximg_to_matrix(png_file);
	if (img.rows() * img.cols() == 0) {
		cerr << "Cannot read image\n";
		return 1;
	}

	auto symbol_groups = split_into_symbol_groups<3>(img);

	if constexpr (debug) {
		show_matrix(img);
		binshow_matrix(img);
		for (size_t i = 0; i < symbol_groups.size(); ++i) {
			cerr << "symbol_groups[" << i << "]:\n";
			for (auto&& symbol : symbol_groups[i])
				binshow_matrix(symbol);
		}
	}

	constexpr double MATCH_THRESHOLD = 10;

	struct DpState {
		bool possible = false;
		int symbol_group;
		string last_symbol;
	};

	int n = symbol_groups[0].size();
	if (n == 0)
		return 0; // Nothing is in the image

	struct BestMatch {
		double best_match_diff = 1e9;
		int best_match_group = -1;
		string best_match_tex;
	};

	vector<DpState> dp(n);
	deque<BestMatch> best_match;
	for (int pos = 0; pos < n; ++pos) {
		best_match.emplace_front();
		if (best_match.size() > symbol_groups.size())
			best_match.pop_back();

		auto& [best_diff, best_match_group, best_match_tex] =
		   best_match.front();

		cerr << "\nSYMBOL No. " << pos << ":\n";

		// Try to match as gr-th symbol group (gr in increasing order)
		for (int gr = 0; gr < min(pos + 1, (int)symbol_groups.size()); ++gr) {
			if (pos > gr and not dp[pos - gr - 1].possible)
				continue;

			auto const& curr_symbol = symbol_groups[gr][pos - gr];

			best_diff = 1e9;
			best_match_group = gr; // In case there are no sdb.symbols()
			best_match_tex = "";
			Matrix<int> best_symbol(0, 0);

			for (auto const& [symbol, tex] : sdb.symbols()) {
				double diff = sdb.statistics().img_diff(curr_symbol, symbol);
				if (diff < best_diff) {
					best_diff = diff;
					best_match_tex = tex;
					best_symbol = symbol;
				}
			}

			if (best_diff <= MATCH_THRESHOLD) {
				cerr << "\033[1;32mMatched as group " << gr << ":\033[m "
				     << best_match_tex << " with diff: " << setprecision(6)
				     << fixed << best_diff << '\n';
				binshow_matrix(curr_symbol);
				binshow_matrix(best_symbol);
				dp[pos] = {true, best_match_group, best_match_tex};
			} else if constexpr (debug) {
				cerr << "\033[33mBest match as group " << gr << ":\033[m "
				     << best_match_tex << " with diff: " << setprecision(6)
				     << fixed << best_diff << '\n';
			}
		}

		auto all_prev_k_failed = [&] {
			int beg = pos - (int)symbol_groups.size() + 1;
			if (beg < 0)
				return false; // A longer symbol may match later

			for (int j = beg; j < pos; ++j)
				if (dp[j].possible)
					return false;

			return true;
		};

		// Cannot match
		if (not dp[pos].possible and (pos == n - 1 or all_prev_k_failed())) {
			if constexpr (debug)
				cerr << endl;

			// Find longest matched prefix end
			int longest_matched_prefix_end = 0;
			for (int i = pos; i > 0; --i) {
				if (dp[i].possible) {
					longest_matched_prefix_end = i;
					break;
				}
			}

			int next_candidate_no = 0;
			cerr << "\033[1;31mCannot match any of the candidates:\033[m\n";
			for (int gr = 0; gr < min(pos + 1, (int)symbol_groups.size());
			     ++gr) {
				// Find all rightmost unmatched symbol candidates:
				// E.g. with symbol_groups.size() == 3
				// longest_matched_prefix_end v
				//                    __  __  __ | __  __  __
				// Candidate 1:                    ^--------^
				// Candidate 2:               ^---------^
				// Candidate 3:           ^---------^

				for (int cand_pos = longest_matched_prefix_end + 1;
				     cand_pos >= max(0, longest_matched_prefix_end - gr + 1);
				     --cand_pos) {
					if (cand_pos > 0 and not dp[cand_pos - 1].possible)
						continue;

					auto const& symbol = symbol_groups[gr][cand_pos];
					auto fsym_file = failed_symbol_file(next_candidate_no++);
					ofstream(fsym_file)
					   << SymbolDatabase::symbol_to_text(symbol);
					cerr << "Candidate from group " << gr << " (saved to "
					     << fsym_file << "):\n";
					binshow_matrix(symbol);

					// auto const& bm = best_match[(int)symbol_groups.size() -
					// gr - 1]; assert(bm.best_match_group == gr); cerr << "best
					// match (" << setprecision(6) << fixed
					//      << bm.best_match_diff << ") is: " <<
					//      bm.best_match_tex
					//      << '\n';
				}
			}

			return 1;
		}
	}

	// Mark only used symbols
	int pos = n - 1;
	while (pos > 0) {
		int gr = dp[pos].symbol_group;
		for (int i = 1; i <= gr; ++i)
			dp[pos - i].possible = false;

		// Save best matched symbols to the database
		// sdb.add_symbol_and_append_file(symbol_groups[gr][pos - gr],
		// dp[pos].last_symbol, GENERATED_SYMBOLS_DB_FILE);

		pos -= gr + 1;
	}

	vector<string> decoded_symbols;
	for (int i = 0; i < n; ++i)
		if (dp[i].possible)
			decoded_symbols.emplace_back(dp[i].last_symbol);

	cout << symbols_to_tex(decoded_symbols) << '\n';
	return 0;
}
