////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "edge.hpp"
#include <array>
#include <algorithm>
#include <functional>
#include <cstddef>

using Triangle = std::array<Edge, 3>;

auto operator>(Triangle const &triangle, double length) {
	return std::any_of(triangle.begin(), triangle.end(), [=](auto const &edge) {
		return edge > length;
	});
}

template <> struct std::hash<Triangle> {
	std::size_t operator()(Triangle const &triangle) const { return hash<Edge>()(triangle[0]); }
};

#endif
