#pragma once

#include "dsu.h"
#include "matrix.h"

#include <iomanip>
#include <iostream>

class Components {
	Matrix<int> cid_; // For each field keep the number of component it is in
	int components_ = 0;

public:
	explicit Components(const SubmatrixView<int>& mat)
	   : Components(mat, mat.rows(), mat.cols()) {}

	Components(const SubmatrixView<int>& mat, int rows, int cols)
	   : cid_(rows, cols) {
		cid_.fill(-1);

		DSU dsu(0);
		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				constexpr int di[] = {-1, -1, 0};
				constexpr int dj[] = {0, -1, -1};
				int last_joined = -1;
				for (int k = 0; k < 3; ++k) {
					int ii = i + di[k];
					int jj = j + dj[k];
					if (ii < 0 or jj < 0)
						continue;

					int x = cid_[ii][jj];
					if (x != -1) {
						if (last_joined != -1)
							dsu.join(last_joined, x);

						last_joined = x;
						// to_join[to_join_size++] = cid_[ii][jj];
					}
				}

				// Join together everything found
				// e.g. .x
				//      y.
				// x and y have to be joined together
				// for (int k = 1; k < to_join_size; ++k)
				// dsu.join(to_join[k - 1], to_join[k]);

				if (mat[i][j]) {
					// if (to_join_size > 0)
					// cid_[i][j] = dsu.find(to_join[0]);
					if (last_joined != -1)
						cid_[i][j] = dsu.find(last_joined);
					else
						cid_[i][j] = dsu.add();
				}
			}
		}

		// vector<int> cids;
		components_ = dsu.size();

		// Update cids to be valid without dsu.find
		for (int i = 0; i < mat.rows(); ++i)
			for (int j = 0; j < mat.cols(); ++j)
				if (cid_[i][j] != -1)
					cid_[i][j] = dsu.find(cid_[i][j]);
		// cids.emplace_back(cid_[i][j] = dsu.find(cid_[i][j]));

		// sort(cids.begin(), cids.end());
		// cids.erase(unique(cids.begin(), cids.end()), cids.end());
		// components_ = cids.size();

		// for (int i = 0; i < mat.rows(); ++i)
		// 	for (int j = 0; j < mat.cols(); ++j)
		// 		if (int& x = cid_[i][j]; x != -1)
		// 			x = lower_bound(cids.begin(), cids.end(), x) - cids.begin();
	}

	int rows() const noexcept { return cid_.rows(); }

	int cols() const noexcept { return cid_.cols(); }

	int component_id(int i, int j) const noexcept { return cid_[i][j]; }

	int components() const noexcept { return components_; }

	void print() const {
		for (int i = 0; i < rows(); ++i) {
			for (int j = 0; j < cols(); ++j) {
				if (cid_[i][j] != -1)
					std::cerr << std::setw(2) << cid_[i][j];
				else
					std::cerr << "  ";
			}

			std::cerr << std::endl;
		}
	}
};
