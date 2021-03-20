#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "edge.hpp"
#include <array>
#include <algorithm>
#include <functional>
#include <cstddef>

using Triangle = std::array<Edge, 3>;

auto operator>(const Triangle &triangle, double length) {
	return std::any_of(triangle.begin(), triangle.end(), [=](const auto &edge) {
		return edge > length;
	});
};

auto operator!(const Triangle &triangle) {
	return triangle[0].first != triangle[2].second;
}

template <> struct std::hash<Triangle> {
	std::size_t operator()(const Triangle &triangle) const { return hash<Edge>()(triangle[0]); }
};

#endif
