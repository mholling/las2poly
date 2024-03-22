////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "edge.hpp"
#include <array>
#include <cmath>
#include <functional>
#include <cstddef>

using Triangle = std::array<Edge, 3>;

auto operator>(Triangle const &triangle, double width) {
	auto const &[edge0, edge1, edge2] = triangle;
	auto const d0 = *edge0.second - *edge0.first;
	auto const d1 = *edge1.second - *edge1.first;
	auto const d2 = *edge2.second - *edge2.first;
	return d0.norm() * d1.norm() * d2.norm() > std::abs(d0 ^ d1) * width;
}

template <> struct std::hash<Triangle> {
	std::size_t operator()(Triangle const &triangle) const { return hash<Edge>()(triangle[0]); }
};

#endif
