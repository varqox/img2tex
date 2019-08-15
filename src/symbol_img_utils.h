#pragma once

#include "matrix.h"

#include <opencv2/opencv.hpp>

template <class T = int>
Matrix<T> teximg_to_matrix(const char* img_path) {
	cv::Mat img;
	cv::imread(img_path).convertTo(img, CV_64F, 1. / 255);

	Matrix<T> res(img.rows, img.cols);
	for (int i = 0; i < img.rows; ++i) {
		for (int j = 0; j < img.cols; ++j) {
			auto pixel = img.at<cv::Point3_<double>>(i, j);
			res[i][j] = 1 - int(round((pixel.x + pixel.y + pixel.z) / 3));
		}
	}

	return res;
}

SubmatrixView<int> without_empty_borders(const SubmatrixView<int>& mat);

inline std::vector<int> column_sum(const Matrix<int>& mat) {
	std::vector<int> col_sum(mat.cols());
	for (int i = 0; i < mat.rows(); ++i)
		for (int j = 0; j < mat.cols(); ++j)
			col_sum[j] += mat[i][j];

	return col_sum;
}

// Returns [{symbols grouped by 1}, ..., {symbols grouped by N}]
template <size_t N>
std::array<std::vector<Matrix<int>>, N>
split_into_symbol_groups(const Matrix<int>& mat) {
	std::vector<int> col_sum = column_sum(mat);
	col_sum.emplace_back(0); // Guard

	std::array<std::vector<Matrix<int>>, N> symbol_groups;
	std::array<int, N> symbols_beg {{}};
	for (int i = 0; i <= mat.cols(); ++i) {
		// Skip non-empty columns
		if (col_sum[i] != 0)
			continue;

		// Skip empty columns after first empty column
		if (symbols_beg[0] == i) {
			symbols_beg[0] = i + 1;
			continue;
		}

		// Add new symbol groups
		for (int k = N - 1; k > 0; --k) {
			if (symbols_beg[k] != symbols_beg[k - 1]) {
				symbol_groups[k].emplace_back(
				   without_empty_borders(SubmatrixView(mat, 0, symbols_beg[k],
				                                       mat.rows(),
				                                       i - symbols_beg[k]))
				      .to_matrix());
				symbols_beg[k] = symbols_beg[k - 1];
			}
		}

		symbol_groups[0].emplace_back(
		   without_empty_borders(SubmatrixView(mat, 0, symbols_beg[0],
		                                       mat.rows(), i - symbols_beg[0]))
		      .to_matrix());

		symbols_beg[0] = i + 1;
	}

	return symbol_groups;
}

// Returns path of the png_file
std::string tex_to_png_file(const std::string& tex, bool quiet = true);

Matrix<int> tex_to_img_matrix(const std::string& tex);

// Prevents excessive cutting of edges of the equation
Matrix<int> safe_tex_to_img_matrix(const std::string& tex);
