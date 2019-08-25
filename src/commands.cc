#include "commands.h"
#include "symbol_database.h"
#include "untex_img.h"
#include "utilities.h"

#include <algorithm>
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
using std::optional;
using std::setprecision;
using std::string;
using std::vector;

constexpr const char* GENERATED_SYMBOLS_DB_FILE = "generated_symbols.db";
constexpr const char* MANUAL_SYMBOLS_DB_FILE = "manual_symbols.db";

inline string failed_symbol_file(int group) {
	return "symbol_" + std::to_string(group);
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

	double diff = sdb.statistics().img_diff(fir, sec);
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
	sdb.add_symbol_and_append_file(SymbolDatabase::text_img_to_symbol(symbol),
	                               tex, MANUAL_SYMBOLS_DB_FILE);

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

	if (access(GENERATED_SYMBOLS_DB_FILE, F_OK) != 0) {
		cerr << "generated symbols database does not exist. Run \"gen\" "
		        "command first\n";
		return 1;
	}

	SymbolDatabase symbol_db;
	if (access(GENERATED_SYMBOLS_DB_FILE, F_OK) == 0)
		symbol_db.add_from_file(GENERATED_SYMBOLS_DB_FILE);
	if (access(MANUAL_SYMBOLS_DB_FILE, F_OK) == 0)
		symbol_db.add_from_file(MANUAL_SYMBOLS_DB_FILE);

	Matrix<int> img = teximg_to_matrix(png_file);
	if (img.rows() * img.cols() == 0) {
		cerr << "Cannot read image\n";
		return 1;
	}

	return std::visit(
	   overloaded {
	      [](string tex) {
		      cout << tex << '\n';
		      return 0;
	      },
	      [](UntexFailure failure) {
		      cerr << "\033[1;31mCannot match any of the candidates:\033[m\n";
		      int next_candidate_no = 0;
		      for (auto& candidate : failure.unmatched_symbol_candidates) {
			      auto fsym_file = failed_symbol_file(next_candidate_no++);
			      ofstream(fsym_file)
			         << SymbolDatabase::symbol_to_text_img(candidate.img);
			      cerr << "Candidate saved to file " << fsym_file << ":\n";
			      binshow_matrix(candidate.img);
		      }

		      return 1;
	      }},
	   untex_img(img, symbol_db, true));
}

// TODO: automatic spacing between symbols
// TODO: merging \mathrm{} together
// TODO: add indexing symbols to make lookup fast for known symbols
