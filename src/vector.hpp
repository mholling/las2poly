#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstddef>
#include <array>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cmath>
#include <ostream>
#include <utility>

template <std::size_t N>
struct Vector : std::array<double, N> {
	Vector() = delete;

	auto &operator+=(const Vector &v) {
		std::transform(this->begin(), this->end(), v.begin(), this->begin(), std::plus<>());
		return *this;
	}

	auto &operator-=(const Vector &v) {
		std::transform(this->begin(), this->end(), v.begin(), this->begin(), std::minus<>());
		return *this;
	}

	auto &operator*=(const double &d) {
		for (auto &value: *this) value *= d;
		return *this;
	}

	auto &operator/=(const double &d) {
		for (auto &value: *this) value /= d;
		return *this;
	}

	friend auto operator+(const Vector &v1, const Vector &v2) { return Vector(v1) += v2; }
	friend auto operator-(const Vector &v1, const Vector &v2) { return Vector(v1) -= v2; }
	friend auto operator*(const Vector &v, const double &d) { return Vector(v) *= d; }
	friend auto operator/(const Vector &v, const double &d) { return Vector(v) /= d; }

	friend auto operator*(const Vector &v1, const Vector &v2) {
		return std::inner_product(v1.begin(), v1.end(), v2.begin(), 0.0);
	}

	auto sqnorm() const { return *this * *this;}
	auto norm() const { return std::sqrt(sqnorm());}
	auto normalise() { return *this /= norm(); }
};

Vector<3> operator^(const Vector<3> &v1, const Vector<3> &v2) {
	return {{
		v1[1] * v2[2] - v1[2] * v2[1],
		v1[2] * v2[0] - v1[0] * v2[2],
		v1[0] * v2[1] - v1[1] * v2[0]
	}};
}

auto operator^(const Vector<2> &v1, const Vector<2> &v2) {
	return v1[0] * v2[1] - v1[1] * v2[0];
}

auto &operator<<(std::ostream &json, const Vector<2> &vector) {
	auto separator = '[';
	for (const auto &coord: vector)
		json << std::exchange(separator, ',') << coord;
	return json << ']';
}

#endif
