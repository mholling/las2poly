#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "vector.hpp"
#include <cstddef>
#include <array>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>

template <std::size_t N>
struct Matrix {
	std::array<Vector<N>, N> columns;

	auto begin() { return columns.begin(); }
	auto   end() { return columns.end(); }

	auto operator[](std::size_t pos) const { return columns[pos]; }

	auto begin() const { return columns.begin(); }
	auto   end() const { return columns.end(); }

	template <typename T>
	auto &operator+=(const T &t) {
		std::transform(begin(), end(), t.begin(), begin(), std::plus<>());
		return *this;
	}

	template <typename T>
	auto &operator-=(const T &t) {
		std::transform(begin(), end(), t.begin(), begin(), std::minus<>());
		return *this;
	}

	template <typename T>
	auto &operator*=(const T &t) {
		for (auto &column: columns) column *= t;
		return *this;
	}

	template <typename T>
	auto &operator/=(const T &t) {
		for (auto &column: columns) column /= t;
		return *this;
	}

	template <typename T>
	friend auto operator+(const Matrix &m, const T &t) { return Matrix(m) += t; }

	template <typename T>
	friend auto operator-(const Matrix &m, const T &t) { return Matrix(m) -= t; }

	template <typename T>
	friend auto operator*(const Matrix &m, const T &t) { return Matrix(m) *= t; }

	template <typename T>
	friend auto operator/(const Matrix &m, const T &t) { return Matrix(m) /= t; }

	template <typename C>
	static auto statistics(const C &vectors) {
		const auto mean = Vector<N>::mean(vectors);
		const auto covariance = std::accumulate(vectors.begin(), vectors.end(), Matrix(), [&](const auto &sum, const auto &vector) {
			const auto delta = mean - vector;
			Matrix outer_product;
			std::transform(delta.begin(), delta.end(), outer_product.begin(), [&](const auto &value) {
				return delta * value;
			});
			return sum + outer_product;
		}) /= vectors.size();
		return std::make_pair(mean, covariance);
	}
};

#endif
