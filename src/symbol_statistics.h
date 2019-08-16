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
		for (int i = std::max(r - 1, 0), rlim = std::min(r + 2, mat.rows());
		     i < rlim; ++i)
			for (int j = std::max(c - 1, 0), clim = std::min(c + 2, mat.cols());
			     j < clim; ++j)
				res |= bool(mat[i][j]) << bit[i - r + 1][j - c + 1];

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
	double img_diff(const SubmatrixView<int>& first,
	                const SubmatrixView<int>& second) const {
		constexpr bool debug = false;

		// first image will be shifted by (x, y) for each x, y \in [-MAX_OFFSET,
		// MAX_OFFSET] and then compared with second
		int rows = std::max(first.rows(), second.rows());
		int cols = std::max(first.cols(), second.cols());
		Matrix<int> fir(rows + MAX_OFFSET * 2, cols + MAX_OFFSET * 2);
		// second.resized(rows + MAX_OFFSET, cols + MAX_OFFSET);

		auto copy_submatrix_with_offset = [](Matrix<int>& dest,
		                                     const SubmatrixView<int>& src,
		                                     int dr, int dc) {
			dr += MAX_OFFSET;
			dc += MAX_OFFSET;

			dest.fill(0);
			for (int r = 0; r < src.rows(); ++r)
				for (int c = 0; c < src.cols(); ++c)
					dest[r + dr][c + dc] = src[r][c];
		};

		copy_submatrix_with_offset(fir, first, 0, 0);
		// auto first_components = Components(first);
		// auto second_components = Components(second);

		if constexpr (debug) {
			// first_components.print();
			// second_components.print();
			show_matrix(calc_prob_pixels(first));
			show_matrix(calc_prob_pixels(second));
			binshow_matrix(first);
			binshow_matrix(second);
			binshow_matrix(fir);
		}

		int first_filled_pixels = std::count(first.begin(), first.end(), 1);
		int second_filled_pixels = std::count(second.begin(), second.end(), 1);

		auto hard_img_diff_with_offset =
		   [&, diff = Matrix<double>(fir.rows(), fir.cols()),
		    diff_orig = Matrix<double>(fir.rows(), fir.cols()),
		    differ = Matrix<char>(fir.rows(), fir.cols())](int dr,
		                                                   int dc) mutable {
			   dr += MAX_OFFSET;
			   dc += MAX_OFFSET;

			   diff.fill(0);
			   diff_orig.fill(0);
			   differ.fill(0);
			   for (int i = 0; i < diff.rows(); ++i) {
				   for (int j = 0; j < diff.cols(); ++j) {
					   int si = i - dr;
					   int sj = j - dc;
					   int second_ij = (si < 0 or si >= second.rows() or
					                          sj < 0 or sj >= second.cols()
					                       ? 0
					                       : second[si][sj]);

					   if (fir[i][j] == second_ij)
						   continue; // No difference

					   differ[i][j] = 1;
					   double x =
					      prob_pxiel(fir, i, j) - prob_pxiel(second, si, sj);
					   diff_orig[i][j] = x;
				   }
			   }

			   for (int i = 0; i < diff.rows(); ++i)
				   for (int j = 0; j < diff.cols(); ++j)
					   if (differ[i][j])
						   diff[i][j] = std::abs((sum3x3(diff_orig, i, j)));
			   // diff[i][j] = sqr((sum3x3(diff_orig, i, j)));

			   // std::cerr << setprecision(6) << fixed << cum_diff / (cols *
			   // rows)
			   // << std::endl;

			   if constexpr (debug) {
				   std::cerr << "dr: " << dr << " dc: " << dc << '\n';
				   binshow_matrix(differ);
				   binshow_matrix(diff * 100);
				   show_matrix(diff);
			   }

			   // Divide image into components and calculate sum over diff of
			   // component / its size)
			   /*auto sum_of_components_diff =
			      [&diff, debug](const Components& comps, int di, int dj) {
			          struct ComponentStats {
			              int size = 0;
			              double diff_sum = 0;
			          };

			          vector<ComponentStats> components(comps.components());
			          for (int i = 0; i < comps.rows(); ++i) {
			              for (int j = 0; j < comps.cols(); ++j) {
			                  int x = comps.component_id(i, j);
			                  if (x != -1) {
			                      ++components[x].size;
			                      components[x].diff_sum +=
			                         diff[i + di][j + dj];
			                  }
			              }
			          }

			          double res = 0;
			          for (size_t i = 0; i < components.size(); ++i) {
			              auto&& component = components[i];
			              if (component.size > 0) {
			                  double val = component.diff_sum / component.size;
			                  res += val;

			                  if constexpr (debug) {
			                      std::cerr << "component " << i
			                           << ": size = " << component.size
			                           << " diff = " << setprecision(6) << fixed
			                           << component.diff_sum << std::endl;
			                  }
			              }
			          }

			          return res;
			      };*/

			   // double fir_diff = sum_of_components_diff(fir_components, dr,
			   // dc); double sec_diff = sum_of_components_diff(sec_components,
			   // 0, 0);
			   // double fir_diff = sum_of_components_diff(first_components,
			   //                                          MAX_OFFSET,
			   //                                          MAX_OFFSET);
			   // double sec_diff =
			   //    sum_of_components_diff(second_components, dr, dc);

			   double simple = 0;
			   for (double x : diff)
				   simple += x;

			   // double mfill = std::min(first_filled_pixels,
			   // second_filled_pixels) + 1; simple *= 1 + 40 / mfill;

			   if constexpr (debug) {
				   // std::cerr << "fir_diff: " << fir_diff << std::endl;
				   // std::cerr << "sec_diff: " << sec_diff << std::endl;
				   // std::cerr << "sum: " << fir_diff + sec_diff << std::endl;
				   std::cerr << "simple: " << simple << std::endl;
			   }

			   // return fir_diff + sec_diff;
			   return simple;
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
