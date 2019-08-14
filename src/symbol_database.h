#pragma once

#include "job_queue.h"
#include "symbol_img_utils.h"
#include "symbol_statistics.h"

#include <fstream>
#include <thread>

class SymbolDatabase {
	std::vector<std::pair<Matrix<int>, std::string>>
	   symbols_; // (symbol, tex formula)
	SymbolStatistics stats_;

	static void write_symbol(std::ofstream& file, const Matrix<int>& symbol,
	                         const std::string& tex_formula) {
		file << tex_formula.size() << ' ' << tex_formula << ' ' << symbol.rows()
		     << ' ' << symbol.cols() << ' ';

		int rows = symbol.rows();
		int cols = symbol.cols();
		int k = 0;
		uint8_t dt = 0;
		constexpr char digits[] = "0123456789abcdef";
		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				dt |= (symbol[i][j] << k++);
				if (k == 4) {
					file << digits[dt];
					dt = 0;
					k = 0;
				}
			}
		}

		if (k > 0)
			file << digits[dt];

		file << '\n';
	}

	static std::pair<Matrix<int>, std::string>
	read_symbol(std::ifstream& file) {
		int k;
		file >> k;
		if (file.get() != ' ')
			throw std::runtime_error("Read invalid symbol");

		std::string tex(k, ' ');
		file.read(tex.data(), k);
		if (file.gcount() != k)
			throw std::runtime_error("Reading symbol error");

		int rows, cols;
		file >> rows >> cols;

		if (file.get() != ' ')
			throw std::runtime_error("Read invalid symbol");

		std::string data((rows * cols + 3) >> 2, '\0');
		file.read(data.data(), data.size());
		if (file.gcount() != (std::streamsize)data.size())
			throw std::runtime_error("Reading symbol error");

		Matrix<int> mat(rows, cols);
		k = 0;
		auto hex_digit_to_int = [](int c) {
			return (c >= 'a' ? c - 'a' + 10 : c - '0');
		};
		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				mat[i][j] = (hex_digit_to_int(data[k >> 2]) >> (k & 3)) & 1;
				++k;
			}
		}

		return {mat, tex};
	}

	void update_statistics(const Matrix<int>& symbol) {
		for (int i = 0; i < symbol.rows(); ++i)
			for (int j = 0; j < symbol.cols(); ++j)
				stats_.increment(SymbolStatistics::mask(symbol, i, j));
	}

	void add_symbol(const Matrix<int>& symbol, const std::string& tex_formula) {
		symbols_.emplace_back(symbol, tex_formula);
		update_statistics(symbol);
	}

public:
	SymbolDatabase() = default;

	void clear() {
		symbols_.clear();
		stats_.reset();
	}

	void add_from_file(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary);
		while (file.get(), file) {
			file.unget();
			symbols_.emplace_back(read_symbol(file));
			update_statistics(symbols_.back().first);
			file.get(); // '\n'
		}
	}

	void save_to_file(const std::string& filename) const {
		std::ofstream file(filename, std::ios::binary);
		for (auto const& [symbol, tex] : symbols_)
			write_symbol(file, symbol, tex);
	}

	const SymbolStatistics& statistics() const noexcept { return stats_; }

	const decltype(symbols_)& symbols() const noexcept { return symbols_; }

	void add_symbol_and_append_file(const Matrix<int>& symbol,
	                                const std::string& tex_formula,
	                                const std::string& filename) {
		for (auto const& [sym, tex] : symbols_)
			if (sym == symbol)
				return;

		std::ofstream file(filename, std::ios::binary | std::ios::app);
		add_symbol(symbol, tex_formula);
		write_symbol(file, symbol, tex_formula);
	}

	static Matrix<int> text_to_symbol(std::string text) {
		if (not text.empty() and text.back() == '\n')
			text.pop_back();

		int rows = count(text.begin(), text.end(), '\n') + 1;
		int cols = text.size() / rows;
		if ((text.size() - (rows - 1)) % rows !=
		    0) // rows - 1 == '\n' occurrences
			throw std::runtime_error("Text does not contain symbol");

		Matrix<int> res(rows, cols);
		int row = 0;
		int col = 0;
		for (char c : text) {
			if (col == cols) {
				if (c != '\n')
					throw std::runtime_error("Text does not contain symbol");

				++row;
				col = 0;
				continue;
			}

			if (c != '#' and c != ' ')
				throw std::runtime_error("Text does not contain symbol");

			res[row][col++] = (c == '#');
		}

		return res;
	}

	static std::string symbol_to_text(const Matrix<int>& symbol) {
		std::string res;
		for (int i = 0; i < symbol.rows(); ++i) {
			for (int j = 0; j < symbol.cols(); ++j)
				res += " #"[bool(symbol[i][j])];
			res += '\n';
		}

		return res;
	}

private:
	static void generate_tex_symbols(JobQueue<std::string>& job_queue) {
		const std::vector<std::string> greek_letters = {
		   "\\alpha",   "\\nu",      "\\beta",    "\\Xi",         "\\xi",
		   "\\Gamma",   "\\gamma",   "\\Delta",   "\\delta",      "\\Pi",
		   "\\pi",      "\\varpi",   "\\epsilon", "\\varepsilon", "\\rho",
		   "\\varrho",  "\\zeta",    "\\Sigma",   "\\sigma",      "\\varsigma",
		   "\\eta",     "\\tau",     "\\Theta",   "\\theta",      "\\vartheta",
		   "\\Upsilon", "\\upsilon", "\\iota",    "\\Phi",        "\\phi",
		   "\\varphi",  "\\kappa",   "\\chi",     "\\Lambda",     "\\lambda",
		   "\\Psi",     "\\psi",     "\\mu",      "\\Omega",      "\\omega"};

		const std::vector<std::string> small_latin = {
		   "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
		   "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

		const std::vector<std::string> big_latin = {
		   "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
		   "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

		const std::vector<std::string> digits = {"0", "1", "2", "3", "4",
		                                         "5", "6", "7", "8", "9"};

		const std::vector<std::string> operators = {
		   "+", "-", "\\neg", "!", "\\#", ">", "<",
		   // "=",
		   "\\%", "\\doteq", "\\equiv", "\\approx", "\\cong", "\\simeq",
		   "\\sim", "\\propto", "\\neq", "\\ne",
		   // "\\nless",
		   // "\\ngtr",
		   "\\leq", "\\geq",
		   // "\\leqslant",
		   // "\\geqslant",
		   // "\\nleq",
		   // "\\ngeq",
		   // "\\nleqslant",
		   // "\\ngeqslant",
		   "\\prec", "\\succ",
		   // "\\nprec",
		   // "\\nsucc",
		   "\\preceq", "\\succeq",
		   // "\\npreceq",
		   // "\\nsucceq",
		   "\\ll", "\\gg",
		   // "\\lll",
		   // "\\ggg",
		   "\\subset", "\\supset", "\\not\\supset", "\\not\\subset",
		   "\\subseteq", "\\supseteq",
		   // "\\nsubseteq",
		   // "\\nsupseteq",
		   // "\\sqsubset",
		   // "\\sqsupset",
		   "\\sqsubseteq", "\\sqsupseteq", "\\|", "\\parallel",
		   // "\\nparallel",
		   "\\asymp", "\\bowtie", "\\vdash", "\\dashv", "\\in", "\\ni",
		   "\\smile", "\\frown", "\\models", "\\notin", "\\perp", "\\mid",
		   "\\pm", "\\cap", "\\diamond", "\\oplus", "\\mp", "\\cup",
		   "\\bigtriangleup", "\\ominus", "\\times", "\\uplus",
		   "\\bigtriangledown", "\\otimes", "\\div", "\\sqcap",
		   "\\triangleleft", "\\oslash", "\\ast", "\\sqcup", "\\triangleright",
		   "\\odot", "\\star", "\\vee", "\\bigcirc", "\\circ", "\\dagger",
		   "\\wedge", "\\bullet", "\\setminus", "\\ddagger", "\\cdot", "\\wr",
		   "\\amalg",
		   // "\\nleqq",
		   // "\\ngeqq",
		   // "\\lneq",
		   // "\\gneq",
		   // "\\lneqq",
		   // "\\gneqq",
		   // "\\lvertneqq",
		   // "\\gvertneqq",
		   // "\\lnsim",
		   // "\\gnsim",
		   // "\\lnapprox",
		   // "\\gnapprox",
		   // "\\precneqq",
		   // "\\succneqq",
		   // "\\precnsim",
		   // "\\succnsim",
		   // "\\precnapprox",
		   // "\\succnapprox",
		   // "\\nsim",
		   // "\\ncong",
		   // "\\nshortmid",
		   // "\\nshortparallel",
		   // "\\nmid",
		   // "\\nvdash",
		   // "\\nvDash",
		   // "\\nVdash",
		   // "\\nVDash",
		   // "\\ntriangleleft",
		   // "\\ntriangleright",
		   // "\\ntrianglelefteq",
		   // "\\ntrianglerighteq",
		   // "\\nsubseteqq",
		   // "\\nsupseteqq",
		   // "\\subsetneq",
		   // "\\supsetneq",
		   // "\\varsubsetneq",
		   // "\\varsupsetneq",
		   // "\\subsetneqq",
		   // "\\supsetneqq",
		   // "\\varsubsetneqq",
		   // "\\varsupsetneqq",
		   "\\exists", "\\not\\exists",
		   // "\\nexists",
		   "\\forall", "\\lor", "\\land", "\\Longrightarrow",
		   // "\\implies",
		   "\\Rightarrow", "\\Longleftarrow", "\\Leftarrow", "\\iff",
		   "\\Leftrightarrow", "\\top", "\\bot", "\\emptyset",
		   // "\\varnothing",
		   "\\O", "\\not\\perp", "\\angle",
		   // "\\measuredangle",
		   "\\triangle",
		   // "\\square",
		   "\\{", "\\}", "(", ")", "\\lceil", "\\rceil",
		   // "\\ulcorner",
		   // "\\urcorner",
		   "/", "\\backslash", "[", "]", "\\langle", "\\rangle", "\\lfloor",
		   "\\rfloor",
		   // "\\llcorner",
		   // "\\lrcorner",
		   "\\rightarrow", "\\to", "\\longrightarrow", "\\mapsto",
		   "\\longmapsto", "\\leftarrow", "\\gets", "\\longleftarrow",
		   "\\uparrow", "\\Uparrow", "\\downarrow", "\\Downarrow",
		   "\\updownarrow", "\\Updownarrow", "\\partial", "\\imath", "\\Re",
		   "\\nabla",
		   // "\\eth",
		   "\\jmath", "\\Im",
		   // "\\Box",
		   "\\hbar", "\\ell", "\\wp", "\\infty", "\\aleph",
		   // "\\beth",
		   // "\\gimel",
		   "\\sin", "\\arcsin", "\\csc", "\\cos", "\\arccos", "\\sec", "\\tan",
		   "\\arctan", "\\cot", "\\sinh",
		   // "\\operatorname{arsinh}",
		   // "\\operatorname{csch}",
		   // "\\operatorname{arcsch}",
		   "\\cosh",
		   // "\\operatorname{arcosh}",
		   // "\\operatorname{sech}",
		   // "\\operatorname{arsech}",
		   "\\tanh",
		   // "\\operatorname{artanh}",
		   "\\coth",
		   // "\\operatorname{arcoth}"
		};

		for (auto const* vec :
		     {&greek_letters, &small_latin, &big_latin, &digits, &operators})
			for (std::string const& symbol : *vec)
				job_queue.add_job(symbol);

		for (auto const* vec : {&greek_letters, &small_latin, &big_latin})
			for (std::string const& symbol : *vec)
				job_queue.add_job(symbol + "'");

		// for (std::string const& letter : big_latin)
		// job_queue.add_job("\\mathbb{" + letter + "}");

		for (auto const* vec : {&small_latin, &big_latin})
			for (std::string const& letter : *vec)
				job_queue.add_job("\\mathrm{" + letter + "}");

		for (std::string const& d1 : digits) {
			for (std::string const& d2 : digits) {
				job_queue.add_job(d1 + "^" + d2);
				for (std::string const& d3 : digits)
					job_queue.add_job(d1 + "^{" + d2 + d3 + "}");
			}
		}

		for (std::string const& letter : small_latin)
			for (std::string const& digit : digits)
				job_queue.add_job(letter + "_" + digit);

		// for (auto const* vec :
		//      {&greek_letters, &small_latin, &big_latin, &digits}) {
		// 	for (std::string const& letter : *vec) {
		// 		job_queue.add_job("{}_" + letter);
		// 		job_queue.add_job("{}^" + letter);
		// 	}
		// }
	}

public:
	void generate_symbols() {
		symbols_.clear();
		stats_.reset();

		add_symbol(text_to_symbol("########\n"
		                          "        \n"
		                          "########\n"),
		           "=");

		add_symbol(text_to_symbol("############\n"
		                          "            \n"
		                          "############\n"),
		           "=");

		JobQueue<std::string> job_queue(1000);
		std::vector<std::thread> threads(std::thread::hardware_concurrency());

		std::mutex symbols_mutex;
		for (auto& thr : threads) {
			thr = std::thread([&] {
				try {
					for (;;) {
						auto tex = job_queue.get_job();
						auto matrix = tex_to_img_matrix(tex);
						std::lock_guard<std::mutex> guard(symbols_mutex);
						add_symbol(matrix, tex);
					}
				} catch (const decltype(job_queue)::NoMoreJobs&) {
				}
			});
		}

		generate_tex_symbols(job_queue);

		job_queue.signal_no_more_jobs();
		for (auto& thr : threads)
			thr.join();
	}
};
