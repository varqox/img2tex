#pragma once

#include "debug.h"
#include "math.h"
#include "matrix_utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

class SymbolStatistics {
	std::array<int, 1 << 9> stats_ = {};

public:
	void reset() noexcept { std::fill(stats_.begin(), stats_.end(), 0); }

	void increment(int mask) noexcept { ++stats_[mask]; }

	template <class U>
	static int mask(const SubmatrixView<int, U>& mat, int r, int c) {
		constexpr std::array<std::array<int, 3>, 3> bit {{
		   {{0, 1, 2}},
		   {{3, 4, 5}},
		   {{6, 7, 8}},
		}};

		int res = 0;
		int rlim = std::min(r + 2, mat.rows());
		int clim = std::min(c + 2, mat.cols());
		for (int i = std::max(r - 1, 0); i < rlim; ++i) {
			for (int j = std::max(c - 1, 0); j < clim; ++j)
				res |= bool(mat[i][j]) << bit[i - r + 1][j - c + 1];
		}

		return res;
	}

	template <class U>
	static int mask(const Matrix<U>& mat, int r, int c) {
		return mask(SubmatrixView<int, U>(mat), r, c);
	}

	double prob_pxiel(int mask) const noexcept {
		constexpr int center_mask = 1 << 4;
		int w_mask = mask |= center_mask;
		int wo_mask = mask & ~center_mask;
		int w_count = stats_[w_mask];
		int wo_count = stats_[wo_mask];
		if (mask & center_mask)
			++w_count;
		else
			++wo_count;

		return (double)w_count / (w_count + wo_count);
	}

	template <class U>
	double prob_pxiel(const SubmatrixView<int, U>& mat, int r, int c) const
	   noexcept {
		return prob_pxiel(mask(mat, r, c));
	}

	template <class U>
	double prob_pxiel(const Matrix<U>& mat, int r, int c) const noexcept {
		return prob_pxiel(SubmatrixView<int, U>(mat), r, c);
	}

	void print() const {
		for (size_t mask = 0; mask < stats_.size(); ++mask) {
			std::cerr << stats_[mask] << ":\n";
			auto symbol = [](int x) { return (x ? '#' : '.'); };
			std::cerr << symbol(mask & 1) << symbol(mask >> 1 & 1)
			          << symbol(mask >> 2 & 1) << '\n';
			std::cerr << symbol(mask >> 3 & 1) << symbol(mask >> 4 & 1)
			          << symbol(mask >> 5 & 1) << '\n';
			std::cerr << symbol(mask >> 6 & 1) << symbol(mask >> 7 & 1)
			          << symbol(mask >> 8 & 1) << '\n';
		}
	}

	Matrix<double> calc_prob_pixels(const SubmatrixView<int>& mat) const {
		int rows = mat.rows();
		int cols = mat.cols();
		Matrix<double> res(rows, cols);
		for (int i = 0; i < rows; ++i)
			for (int j = 0; j < cols; ++j)
				res[i][j] = prob_pxiel(mat, i, j);

		return res;
	}

	template <size_t MAX_OFFSET = 1>
	double
	img_diff(const SubmatrixView<int>& first,
	         const SubmatrixView<int>& second,
	         double diff_threshold = std::numeric_limits<double>::max()) const {
		constexpr bool debug = false;

		// first image will be shifted by (x, y) for each x, y \in [-MAX_OFFSET,
		// MAX_OFFSET] and then compared with second
		int rows = std::max(first.rows(), second.rows());
		int cols = std::max(first.cols(), second.cols());
		Matrix<int> fir(rows + MAX_OFFSET * 2, cols + MAX_OFFSET * 2);

		auto copy_submatrix_with_offset = [](Matrix<int>& dest,
		                                     const SubmatrixView<int>& src,
		                                     int dr,
		                                     int dc) {
			dr += MAX_OFFSET;
			dc += MAX_OFFSET;

			dest.fill(0);
			for (int r = 0; r < src.rows(); ++r)
				for (int c = 0; c < src.cols(); ++c)
					dest[r + dr][c + dc] = src[r][c];
		};

		copy_submatrix_with_offset(fir, first, 0, 0);

		if constexpr (debug) {
			show_matrix(calc_prob_pixels(first));
			show_matrix(calc_prob_pixels(second));
			binshow_matrix(first);
			binshow_matrix(second);
			binshow_matrix(fir);
		}

		constexpr double DIFFERING_CELL_PENALTY = 1e-3;

		auto hard_img_diff_with_offset =
		   [&,
		    diff = Matrix<double>(fir.rows(), fir.cols()),
		    diff_orig = Matrix<double>(fir.rows(), fir.cols()),
		    differ = Matrix<char>(fir.rows(), fir.cols())](int dr,
		                                                   int dc) mutable {
			   dr += MAX_OFFSET;
			   dc += MAX_OFFSET;

			   diff.fill(0);
			   diff_orig.fill(0);
			   differ.fill(0);

			   double diff_sum = 0;
			   // Returns true iff. the sum has exceeded the threshold
			   auto update_diff_sum = [&](int r, int c) {
				   if (r < 0 or c < 0 or not differ[r][c])
					   return false;

				   diff[r][c] = std::abs(sum3x3(diff_orig, r, c));
				   diff_sum += diff[r][c];
				   return (diff_sum > diff_threshold);
			   };
			   for (int i = 0; i < diff.rows(); ++i) {
				   for (int j = 0; j < diff.cols(); ++j) {
					   if (update_diff_sum(i - 1, j - 2))
						   return diff_sum;

					   int si = i - dr;
					   int sj = j - dc;
					   int second_ij = (si < 0 or si >= second.rows() or
					                          sj < 0 or sj >= second.cols()
					                       ? 0
					                       : second[si][sj]);

					   if (fir[i][j] == second_ij)
						   continue; // No difference

					   differ[i][j] = 1;
					   diff_sum += DIFFERING_CELL_PENALTY;

					   diff_orig[i][j] =
					      prob_pxiel(fir, i, j) - prob_pxiel(second, si, sj);
				   }

				   if (update_diff_sum(i - 1, diff.cols() - 2))
					   return diff_sum;
				   if (update_diff_sum(i - 1, diff.cols() - 1))
					   return diff_sum;
			   }

			   for (int i = diff.rows() - 1, j = 0; j < diff.cols(); ++j) {
				   if (update_diff_sum(i, j))
					   return diff_sum;
			   }

			   if constexpr (debug) {
				   std::cerr << "dr: " << dr << " dc: " << dc << '\n';
				   binshow_matrix(differ);
				   binshow_matrix(diff * 100);
				   show_matrix(diff);
			   }

			   if constexpr (debug)
				   std::cerr << "simple: " << diff_sum << std::endl;

			   return diff_sum;
		   };

		double min_diff = std::numeric_limits<double>::max();
		for (int dr = -(int)MAX_OFFSET; dr <= (int)MAX_OFFSET; ++dr)
			for (int dc = -(int)MAX_OFFSET; dc <= (int)MAX_OFFSET; ++dc)
				min_diff =
				   std::min(min_diff, hard_img_diff_with_offset(dr, dc));

		if constexpr (debug)
			std::cerr << "min_diff: " << min_diff << std::endl;

		return min_diff;
	}
};
