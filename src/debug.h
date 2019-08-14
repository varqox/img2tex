#pragma once

#include "matrix.h"

#include <iomanip>
#include <iostream>

template <class U>
void binshow_matrix(const SubmatrixView<int, U>& mat) {
	std::cerr << mat.rows() << ' ' << mat.cols() << std::endl;
	for (int i = 0; i < mat.rows(); ++i) {
		for (int j = 0; j < mat.cols(); ++j)
			std::cerr << (mat[i][j] ? '#' : ' ');
		std::cerr << std::endl;
	}
}

template <class T>
void binshow_matrix(const Matrix<T>& mat) {
	return binshow_matrix(SubmatrixView<int, T>(mat));
}

template <class T, class U>
void show_matrix(const SubmatrixView<T, U>& mat) {
	std::cerr << mat.rows() << ' ' << mat.cols() << std::endl;
	for (int i = 0; i < mat.rows(); ++i) {
		for (int j = 0; j < mat.cols(); ++j) {
			if constexpr (std::is_floating_point_v<T>) {
				if (mat.at(i, j) >= 0.0005) {
					std::cerr << std::setw(3) << std::setprecision(3)
					          << mat.at(i, j) << ' ';
				} else {
					std::cerr << "      ";
				}
			} else {
				std::cerr << mat.at(i, j) << ' ';
			}
		}
		std::cerr << std::endl;
	}
}

template <class T>
void show_matrix(const Matrix<T>& mat) {
	return show_matrix(SubmatrixView(mat));
}
