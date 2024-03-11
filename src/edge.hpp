////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef EDGE_HPP
#define EDGE_HPP

#include "points.hpp"
#include "link.hpp"
#include <utility>
#include <compare>
#include <functional>
#include <cstddef>

using Edge = std::pair<PointIterator, PointIterator>;

// edge <  point : edge lies to the left of point
// edge <= point : edge lies to the left of point or is colinear
// edge >= point : edge lies to the right of point or is colinear
// edge >  point : edge lies to the right of point

auto operator<=>(Edge const &edge, PointIterator const &point) {
	auto const &[p1, p2] = edge;
	return Link(*p1, *p2) <=> *point;
}

auto operator^(Edge const &edge1, Edge const &edge2) { // 3d cross product
	return (+*edge1.second - +*edge1.first) ^ (+*edge2.second - +*edge2.first);
}

auto operator^(Edge const &edge, PointIterator const &point) { // 2d cross product
	return (*edge.second - *edge.first) ^ (*point - *edge.first);
}

auto operator>(Edge const &edge, double length) {
	return (*edge.second - *edge.first).sqnorm() > length * length;
}

auto operator-(Edge const &edge) {
	return Edge(edge.second, edge.first);
}

template <> struct std::hash<Edge> {
	std::size_t operator()(Edge const &edge) const {
		auto static constexpr hash = std::hash<PointIterator>();
		auto const seed = hash(edge.first);
		return seed ^ (hash(edge.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
