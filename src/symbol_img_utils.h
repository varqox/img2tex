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

template <class T = int>
void save_binary_image_to(const Matrix<T>& img, const char* img_path) {
	cv::Mat out(img.rows(), img.cols(), CV_32FC4);
	for (int i = 0; i < img.rows(); ++i) {
		for (int j = 0; j < img.cols(); ++j) {
			float x = (1 - img[i][j]) * 255.;
			float alpha = img[i][j] * 255.;
			out.at<cv::Vec4f>(i, j) = {x, x, x, alpha};
		}
	}

	cv::imwrite(img_path, out);
}

struct WithoutBordersRes {
	SubmatrixView<int> symbol;
	int top_rows_cut;
	int bottom_rows_cut;
};

WithoutBordersRes without_empty_borders(const SubmatrixView<int>& mat);

inline std::vector<int> column_sum(const Matrix<int>& mat) {
	std::vector<int> col_sum(mat.cols());
	for (int i = 0; i < mat.rows(); ++i)
		for (int j = 0; j < mat.cols(); ++j)
			col_sum[j] += mat[i][j];

	return col_sum;
}

struct SplitSymbol {
	Matrix<int> img;
	int first_column_pos;
	int top_rows_cut;
	int bottom_rows_cut;
};

// Returns [{symbols grouped by 1}, ..., {symbols grouped by N}]
template <size_t N>
std::array<std::vector<SplitSymbol>, N>
split_into_symbol_groups(const Matrix<int>& mat) {
	std::vector<int> col_sum = column_sum(mat);
	col_sum.emplace_back(0); // Guard

	std::array<std::vector<SplitSymbol>, N> symbol_groups;
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
				auto res = without_empty_borders(SubmatrixView(
				   mat, 0, symbols_beg[k], mat.rows(), i - symbols_beg[k]));
				symbol_groups[k].push_back({res.symbol.to_matrix(),
				                            symbols_beg[k],
				                            res.top_rows_cut,
				                            res.bottom_rows_cut});
				symbols_beg[k] = symbols_beg[k - 1];
			}
		}

		auto res = without_empty_borders(SubmatrixView(
		   mat, 0, symbols_beg[0], mat.rows(), i - symbols_beg[0]));
		symbol_groups[0].push_back({res.symbol.to_matrix(),
		                            symbols_beg[0],
		                            res.top_rows_cut,
		                            res.bottom_rows_cut});

		symbols_beg[0] = i + 1;
	}

	return symbol_groups;
}

int symbol_horizontal_distance(const SplitSymbol& fir, const SplitSymbol& sec);

// Returns path of the png_file
std::string tex_to_png_file(const std::string& tex, bool quiet = true);

Matrix<int> tex_to_img_matrix(const std::string& tex);

// Prevents excessive cutting of edges of the equation
Matrix<int> safe_tex_to_img_matrix(const std::string& tex);
