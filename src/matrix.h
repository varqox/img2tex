#pragma once

#include <cassert>
#include <vector>

template <class T = int>
class Matrix {
	int n, m;
	std::vector<T> data;

	template <class U>
	friend class Matrix;

public:
	Matrix(int rows, int cols) : n(rows), m(cols), data(rows * cols) {}

	Matrix(const Matrix&) = default;
	Matrix(Matrix&&) noexcept = default;
	Matrix& operator=(const Matrix&) = default;
	Matrix& operator=(Matrix&&) noexcept = default;

	template <class U, class = std::enable_if_t<std::is_convertible_v<U, T>>>
	Matrix(const Matrix<U>& other)
	   : n(other.n), m(other.m), data(other.data.begin(), other.data.end()) {}

	template <class U, class = std::enable_if_t<std::is_convertible_v<U, T>>>
	Matrix& operator=(const Matrix<U>& other) {
		n = other.n;
		m = other.m;
		data.assign(other.data.begin(), other.data.end());
		return *this;
	}

	Matrix resized(int rows, int cols) const {
		Matrix res(rows, cols);
		int rend = std::min(n, rows);
		int cend = std::min(m, cols);
		for (int r = 0; r < rend; ++r)
			for (int c = 0; c < cend; ++c)
				res[r][c] = (*this)[r][c];

		return res;
	}

	int rows() const noexcept { return n; }

	int cols() const noexcept { return m; }

	T& at(int i, int j) noexcept { return data[m * i + j]; }

	const T& at(int i, int j) const noexcept { return data[m * i + j]; }

	T* operator[](int i) noexcept { return data.data() + m * i; }

	const T* operator[](int i) const noexcept { return data.data() + m * i; }

	auto begin() noexcept { return data.begin(); }

	auto begin() const noexcept { return data.begin(); }

	auto end() noexcept { return data.end(); }

	auto end() const noexcept { return data.end(); }

	Matrix& fill(T val) noexcept {
		std::fill(data.begin(), data.end(), val);
		return *this;
	}

	friend bool operator==(const Matrix& a, const Matrix& b) {
		if (a.rows() != b.rows() or a.cols() != b.cols())
			return false;

		for (size_t i = 0; i < a.data.size(); ++i)
			if (a.data[i] != b.data[i])
				return false;

		return true;
	}

	friend bool operator!=(const Matrix& a, const Matrix& b) {
		return not(a == b);
	}

	friend Matrix operator+(const Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] + b.data[i];

		return res;
	}

	friend Matrix& operator+=(Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] += b.data[i];

		return a;
	}

	friend Matrix operator-(const Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] - b.data[i];

		return res;
	}

	friend Matrix& operator-=(Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] -= b.data[i];

		return a;
	}

	friend Matrix operator&(const Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] & b.data[i];

		return res;
	}

	friend Matrix& operator&=(Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] &= b.data[i];

		return a;
	}

	friend Matrix operator|(const Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] | b.data[i];

		return res;
	}

	friend Matrix& operator|=(Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] |= b.data[i];

		return a;
	}

	friend Matrix operator^(const Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] ^ b.data[i];

		return res;
	}

	friend Matrix& operator^=(Matrix& a, const Matrix& b) {
		assert(a.n == b.n and a.m == b.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] ^= b.data[i];

		return a;
	}

	friend Matrix operator*(const Matrix& a, const T& val) {
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] * val;

		return res;
	}

	friend Matrix& operator*=(Matrix& a, const T& val) {
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] *= val;

		return a;
	}

	friend Matrix operator/(const Matrix& a, const T& val) {
		Matrix res(a.n, a.m);
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			res.data[i] = a.data[i] / val;

		return res;
	}

	friend Matrix& operator/=(Matrix& a, const T& val) {
		for (int i = 0, end = a.n * a.m; i < end; ++i)
			a.data[i] /= val;

		return a;
	}
};

template <class T, class U = T>
class SubmatrixView {
	static_assert(std::is_convertible_v<U, T>,
	              "Cannot create view of an incompatible matrix");
	const Matrix<U>& mat_;
	const int beg_row_;
	const int beg_col_;
	const int rows_;
	const int cols_;

	template <class A, class B>
	friend class SubmatrixView;

public:
	SubmatrixView(const Matrix<U>& matrix,
	              int beg_row,
	              int beg_col,
	              int rows,
	              int cols)
	   : mat_(matrix), beg_row_(beg_row), beg_col_(beg_col), rows_(rows),
	     cols_(cols) {}

	SubmatrixView(const Matrix<U>& matrix)
	   : SubmatrixView(matrix, 0, 0, matrix.rows(), matrix.cols()) {}

	SubmatrixView(const SubmatrixView<U>& submatrix,
	              int beg_row,
	              int beg_col,
	              int rows,
	              int cols)
	   : SubmatrixView(submatrix.mat_,
	                   submatrix.beg_row_ + beg_row,
	                   submatrix.beg_col_ + beg_col,
	                   rows,
	                   cols) {}

	template <class V>
	SubmatrixView(const SubmatrixView<V, U>& other)
	   : SubmatrixView(other.mat_,
	                   other.beg_row_,
	                   other.beg_col_,
	                   other.rows_,
	                   other.cols_) {}

	Matrix<T> resized(int rows, int cols) const {
		Matrix res(rows, cols);
		int rend = std::min(rows_, rows);
		int cend = std::min(cols_, cols);
		for (int r = 0; r < rend; ++r)
			for (int c = 0; c < cend; ++c)
				res[r][c] = (*this)[r][c];

		return res;
	}

	Matrix<T> to_matrix() const { return resized(rows_, cols_); }

	int rows() const noexcept { return rows_; }

	int cols() const noexcept { return cols_; }

	const T& at(int i, int j) const noexcept {
		return mat_[beg_row_ + i][beg_col_ + j];
	}

	const U* operator[](int i) const noexcept {
		return mat_[beg_row_ + i] + beg_col_;
	}

	template <class Type>
	class Iterator {
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = Type;
		using difference_type = int;
		using pointer = value_type*;
		using reference = value_type&;

	private:
		using SubmatrixViewPtr = std::conditional_t<std::is_const_v<Type>,
		                                            const SubmatrixView*,
		                                            SubmatrixView*>;
		SubmatrixViewPtr sv = nullptr;
		int row = 0, col = 0;

		Iterator(SubmatrixViewPtr s, int r, int c) : sv(s), row(r), col(c) {}

		friend class SubmatrixView;

	public:
		Iterator() = default;

		reference operator*() const noexcept { return (*sv)[row][col]; }

		pointer operator->() const noexcept { return &(*sv)[row][col]; }

		Iterator& operator++() noexcept {
			if (++col == sv->cols()) {
				++row;
				col = 0;
			}

			return *this;
		}

		Iterator operator++(int) noexcept {
			Iterator it = *this;
			++*this;
			return it;
		}

		Iterator& operator--() noexcept {
			if (col == 0) {
				--row;
				col = sv->cols() - 1;
			} else {
				--col;
			}

			return *this;
		}

		Iterator operator--(int) noexcept {
			Iterator it = *this;
			--*this;
			return it;
		}

		friend bool operator==(Iterator a, Iterator b) noexcept {
			return (a.sv == b.sv and a.row == b.row and a.col == b.col);
		}

		friend bool operator!=(Iterator a, Iterator b) noexcept {
			return not(a == b);
		}
	};

	using iterator = Iterator<T>;
	using const_iterator = Iterator<const T>;

	iterator begin() noexcept { return iterator(this, 0, 0); }

	const_iterator begin() const noexcept { return const_iterator(this, 0, 0); }

	iterator end() noexcept { return iterator(this, rows(), 0); }

	const_iterator end() const noexcept {
		return const_iterator(this, rows(), 0);
	}
};

template <class T, class... Args>
SubmatrixView(const Matrix<T>& mat, Args&&...)->SubmatrixView<T, T>;
