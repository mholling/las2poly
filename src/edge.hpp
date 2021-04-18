////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef EDGE_HPP
#define EDGE_HPP

#include "points.hpp"
#include "exact.hpp"
#include <utility>
#include <limits>
#include <cmath>
#include <algorithm>
#include <compare>
#include <functional>
#include <cstddef>

using Edge = std::pair<PointIterator, PointIterator>;

// edge <  point : point lies to the right of edge
// edge <= point : point lies to the right of edge or is colinear
// edge >= point : point lies to the left of edge or is colinear
// edge >  point : point lies to the left of edge

auto operator<=>(Edge const &edge, PointIterator const &point) {
	auto static constexpr epsilon = 0.5 * std::numeric_limits<double>::epsilon();
	auto static constexpr error_scale = epsilon * (3 + 16 * epsilon);

	auto const &[p1, p2] = edge;
	auto const &[x1, y1] = *p1;
	auto const &[x2, y2] = *p2;
	auto const &[x3, y3] = *point;

	auto const det1 = (x2 - x1) * (y3 - y2);
	auto const det2 = (x3 - x2) * (y2 - y1);
	auto const det = det1 - det2;

	if (std::abs(det) > error_scale * (std::abs(det1) + std::abs(det2)))
		return det <=> 0;

	auto const [x_min, x_max] = std::minmax({x1, x2, x3});
	auto const [y_min, y_max] = std::minmax({y1, y2, y3});

	if ((2 * x_min > x_max || 2 * x_max < x_min) && (2 * y_min > y_max || 2 * y_max < y_min)) {
		auto const det1 = Exact(x2 - x1) * Exact(y3 - y2);
		auto const det2 = Exact(x3 - x2) * Exact(y2 - y1);
		return det1 - det2 <=> 0;
	} else {
		auto const det1 = Exact(x1) * Exact(y2) - Exact(x2) * Exact(y1);
		auto const det2 = Exact(x2) * Exact(y3) - Exact(x3) * Exact(y2);
		auto const det3 = Exact(x3) * Exact(y1) - Exact(x1) * Exact(y3);
		return det1 + det2 + det3 <=> 0;
	}
}

auto operator^(Edge const &edge1, Edge const &edge2) { // 3d cross product
	return (+*edge1.first - +*edge1.second) ^ (+*edge2.first - +*edge2.second);
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
