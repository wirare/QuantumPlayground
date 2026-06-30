#pragma once

#include <ostream>
#include <vector>

template <typename K>
class Matrix
{
	public:
		Matrix(const std::vector<K> &vec, size_t rows, size_t cols): data(vec.begin(), vec.end()), rows(rows), cols(cols) {} 
		Matrix(size_t rows, size_t cols): data(rows * cols), rows(rows), cols(cols) {}
		Matrix(size_t rows, size_t cols, const K &value): data(rows * cols, value), rows(rows), cols(cols) {}
		Matrix(size_t rows, size_t cols, const std::initializer_list<K> &init): data(init), rows(rows), cols(cols)
		{
			if (data.size() != rows * cols)
				throw std::invalid_argument("Matrix initializer size does not match rows * cols");
		}

		size_t size() const { return data.size(); }
		const std::vector<K> &getData() const { return data; }

		friend std::ostream& operator<<(std::ostream& os, const Matrix<K>& m) 
		{
			os << "Matrix(" << m.rows << "x" << m.cols << ") [\n";
			for (size_t i = 0; i < m.rows; ++i) 
			{
				os << "  [";
				if (m.cols > 0) 
				{
					os << m.data[i * m.cols];
					for (size_t j = 1; j < m.cols; ++j) 
					{
						os << ", " << m.data[i * m.cols + j];
					}
				}
				os << "]";
				if (i + 1 < m.rows) 
					os << ",";
				os << "\n";
			}
			os << "]";
			return os;
		}

		Matrix mul_mat(const Matrix &mat) const
		{
			if (cols != mat.rows)
				throw std::invalid_argument("Matrix A must have the same number of cols as Matrix B has rows");

			Matrix res(rows, mat.cols);

			std::fill(res.data.begin(), res.data.end(), K{});

			for (size_t i = 0; i < rows; ++i)
			{
				for (size_t k = 0; k < cols; ++k)
				{
					const K a = data[i * cols + k];

					for (size_t j = 0; j < mat.cols; ++j)
						res.data[i * res.cols + j] += a * mat.data[k * mat.cols + j];
				}
			}

			return res;
		}

		size_t row_count() const { return rows; }
		size_t col_count() const { return cols; }
		K& operator()(size_t row, size_t col) { return data[row * cols + col]; }
		const K& operator()(size_t row, size_t col) const { return data[row * cols + col]; }
		Matrix operator*(const Matrix& other) const { return mul_mat(other); }

	private:
		std::vector<K> data;
		size_t rows;
		size_t cols;
};
