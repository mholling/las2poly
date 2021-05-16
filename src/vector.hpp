////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstddef>
#include <array>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cmath>
#include <tuple>
#include <type_traits>

template <std::size_t N>
struct Vector : std::array<double, N> {
	auto &operator+=(Vector const &v) {
		std::transform(this->begin(), this->end(), v.begin(), this->begin(), std::plus<>());
		return *this;
	}

	auto &operator-=(Vector const &v) {
		std::transform(this->begin(), this->end(), v.begin(), this->begin(), std::minus<>());
		return *this;
	}

	auto &operator*=(double const &d) {
		for (auto &value: *this) value *= d;
		return *this;
	}

	auto &operator/=(double const &d) {
		for (auto &value: *this) value /= d;
		return *this;
	}

	friend auto operator+(Vector const &v1, Vector const &v2) { return Vector(v1) += v2; }
	friend auto operator-(Vector const &v1, Vector const &v2) { return Vector(v1) -= v2; }
	friend auto operator*(Vector const &v, double const &d) { return Vector(v) *= d; }
	friend auto operator/(Vector const &v, double const &d) { return Vector(v) /= d; }

	friend auto operator*(Vector const &v1, Vector const &v2) {
		return std::inner_product(v1.begin(), v1.end(), v2.begin(), 0.0);
	}

	auto sqnorm() const { return *this * *this;}
	auto norm() const { return std::sqrt(sqnorm());}
	auto normalise() { return *this /= norm(); }
};

Vector<3> operator^(Vector<3> const &v1, Vector<3> const &v2) {
	return {{
		v1[1] * v2[2] - v1[2] * v2[1],
		v1[2] * v2[0] - v1[0] * v2[2],
		v1[0] * v2[1] - v1[1] * v2[0]
	}};
}

auto operator^(Vector<2> const &v1, Vector<2> const &v2) {
	return v1[0] * v2[1] - v1[1] * v2[0];
}

template <std::size_t N>
struct std::tuple_size<Vector<N>> : std::integral_constant<std::size_t, N> { };

template <std::size_t N, std::size_t M>
struct std::tuple_element<M, Vector<N>> { using type = double; };

#endif
