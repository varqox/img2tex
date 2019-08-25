#include "symbol_img_utils.h"
#include "defer.h"
#include "run_command.h"
#include "temporary_file.h"

#include <fstream>

using std::max;
using std::min;
using std::string;

WithoutBordersRes without_empty_borders(const SubmatrixView<int>& mat) {
	int rows = mat.rows();
	int cols = mat.cols();

	int min_row = rows;
	int max_row = 0;
	int min_col = cols;
	int max_col = 0;

	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			if (mat[r][c]) {
				min_row = min(min_row, r);
				max_row = max(max_row, r);
				min_col = min(min_col, c);
				max_col = max(max_col, c);
			}
		}
	}

	if (min_row == rows)
		return {SubmatrixView<int>(mat, 0, 0, 0, 0), rows / 2, (rows + 1) / 2};

	return {SubmatrixView<int>(mat, min_row, min_col, max_row - min_row + 1,
	                           max_col - min_col + 1),
	        min_row, rows - 1 - max_row};
}

// Returns path of the png_file
string tex_to_png_file(const string& tex, bool quiet) {
	TemporaryFile tex_file("/tmp/texXXXXXX");
	std::ofstream(tex_file.path()) << "\\documentclass[12pt,polish]{article}\n"
	                                  "\\pagestyle{empty}\n"
	                                  "\\usepackage{mathtools}\n"
	                                  "\\begin{document}\n"
	                                  "\\begin{displaymath}\n"
	                               << tex
	                               << "\\end{displaymath}\n"
	                                  "\\end{document}\n";

	string dvi_filename = tex_file.path() + ".dvi";
	string ps_filename = tex_file.path() + ".ps";
	string aux_filename = tex_file.path() + ".aux";
	string log_filename = tex_file.path() + ".log";
	string png_filename = tex_file.path() + ".png";

	bool remove_png_file = true;
	Defer guard([&] {
		unlink(dvi_filename.data());
		unlink(ps_filename.data());
		unlink(aux_filename.data());
		unlink(log_filename.data());
		if (remove_png_file)
			unlink(png_filename.data());
	});

	auto run = [&](auto&&... args) {
		return run_command(quiet, std::forward<decltype(args)>(args)...);
	};

	if (not run("latex", "-output-directory=/tmp", tex_file.path()) or
	    not run("dvips", dvi_filename, "-o", ps_filename) or
	    not run("pstoimg", "-interlaced", "-transparent", "-scale", "1.4",
	            "-crop", "as", "-type", "png", "-out", png_filename,
	            ps_filename))
		throw std::runtime_error("Failed to convert tex to png: " + tex);

	remove_png_file = false;
	return png_filename;
}

Matrix<int> tex_to_img_matrix(const string& tex) {
	string png_filename = tex_to_png_file(tex);
	Defer guard([&] { unlink(png_filename.data()); });
	return teximg_to_matrix(png_filename.data());
}

Matrix<int> safe_tex_to_img_matrix(const std::string& tex) {
	// Add integral symbol at the beginning and end to prevent cutting the edges
	// of the @p tex
	auto matrix = tex_to_img_matrix("\\int\\,\\, " + tex + " \\,\\,\\int");
	// Now we have to cut out the integral symbols
	std::vector<int> col_sum = column_sum(matrix);
	auto beg = std::find(col_sum.begin(), col_sum.end(), 0);
	if (beg == col_sum.end()) {
		throw std::runtime_error(
		   "safe_tex_to_img_matrix(): spacing equation does not work");
	}

	auto end = std::find(col_sum.rbegin(), col_sum.rend(), 0);
	int first_empty_column = beg - col_sum.begin();
	int last_empty_column = --end.base() - col_sum.begin();

	return without_empty_borders(
	          SubmatrixView(matrix, 0, first_empty_column, matrix.rows(),
	                        last_empty_column - first_empty_column))
	   .symbol.to_matrix();
}
