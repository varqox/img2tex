#pragma once

#include "matrix.h"

template <class T, class U>
T sum3x3(const SubmatrixView<T, U>& mat, int r, int c) {
	T sum = T();
	int rlim = std::min(r + 2, mat.rows());
	int clim = std::min(c + 2, mat.cols());
	for (int i = std::max(r - 1, 0); i < rlim; ++i) {
		for (int j = std::max(c - 1, 0); j < clim; ++j)
			sum += mat[i][j];
	}

	return sum;
}

template <class T>
T sum3x3(const Matrix<T>& mat, int r, int c) {
	return sum3x3(SubmatrixView(mat), r, c);
}

template <class T, class U>
int size3x3(const SubmatrixView<T, U>& mat, int r, int c) {
	int rbeg = std::max(r - 1, 0);
	int rend = std::min(r + 2, mat.rows());
	int cbeg = std::max(c - 1, 0);
	int cend = std::min(c + 2, mat.cols());
	return (rend - rbeg) * (cend - cbeg);
}

template <class R, class T, class U>
R avg3x3(const SubmatrixView<T, U>& mat, int r, int c) {
	return R(sum3x3(mat, r, c)) / size3x3(mat, r, c);
}

template <class R, class T, class U>
R absavg3x3(const SubmatrixView<T, U>& mat, int r, int c) {
	return R(sum3x3(mat, r, c)) / 9;
}
