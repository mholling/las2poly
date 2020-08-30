#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstddef>
#include <array>
#include <fstream>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cmath>

template <std::size_t N>
struct Vector {
	std::array<double, N> values = {};

	auto begin() { return values.begin(); }
	auto   end() { return values.end(); }

	auto begin() const { return values.begin(); }
	auto   end() const { return values.end(); }

	auto operator[](std::size_t pos) const { return values[pos]; }

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
		for (auto &value: values) value *= t;
		return *this;
	}

	template <typename T>
	auto &operator/=(const T &t) {
		for (auto &value: values) value /= t;
		return *this;
	}

	template <typename T>
	friend auto operator+(const Vector &v, const T &t) { return Vector(v) += t; }

	template <typename T>
	friend auto operator-(const Vector &v, const T &t) { return Vector(v) -= t; }

	template <typename T>
	friend auto operator*(const Vector &v, const T &t) { return Vector(v) *= t; }

	template <typename T>
	friend auto operator/(const Vector &v, const T &t) { return Vector(v) /= t; }

	friend auto operator+(const Vector &v) { return v; }

	friend auto operator-(const Vector &v) { return v * -1; }

	friend auto operator*(const Vector &v1, const Vector &v2) {
		return std::inner_product(v1.begin(), v1.end(), v2.begin(), 0.0);
	}

	auto sqnorm() const { return *this * *this;}
	auto norm() const { return std::sqrt(sqnorm());}
	auto normalise() { return (*this) /= norm(); }
};

Vector<3> operator^(const Vector<3> &v1, const Vector<3> &v2) {
	return {
		v1[1] * v2[2] - v1[2] * v2[1],
		v1[2] * v2[0] - v1[0] * v2[2],
		v1[0] * v2[1] - v1[1] * v2[0]
	};
}

auto operator^(const Vector<2> &v1, const Vector<2> &v2) {
	return v1[0] * v2[1] - v1[1] * v2[0];
}

#endif
