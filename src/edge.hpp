#ifndef EDGE_HPP
#define EDGE_HPP

#include "points.hpp"
#include "exact.hpp"
#include <utility>
#include <cmath>
#include <compare>
#include <functional>
#include <cstddef>

using Edge = std::pair<PointIterator, PointIterator>;

auto operator^(Edge const &edge1, Edge const &edge2) {
	auto const [x1, y1] = *edge1.second - *edge1.first;
	auto const [x2, y2] = *edge2.second - *edge2.first;
	return Exact(x1) * Exact(y2) - Exact(y1) * Exact(x2);
}

auto operator*(Edge const &edge1, Edge const &edge2) {
	auto const [x1, y1] = *edge1.second - *edge1.first;
	auto const [x2, y2] = *edge2.second - *edge2.first;
	return Exact(x1) * Exact(y2) + Exact(y1) * Exact(x2);
}

// edge1 <  edge2 : edge2 turns anticlockwise from edge1
// edge1 <= edge2 : edge2 turns anticlockwise from or is parallel to edge1
// edge1 >= edge2 : edge2 turns clockwise from or is parallel to edge1
// edge1 >  edge2 : edge2 turns clockwise from edge1

auto operator<=>(Edge const &edge1, Edge const &edge2) {
	using std::abs, IEEE754::epsilon;
	auto static constexpr error_scale = epsilon() * (1 + 2 * epsilon());

	auto const [x1, y1] = *edge1.second - *edge1.first;
	auto const [x2, y2] = *edge2.second - *edge2.first;
	auto const det1 = x1 * y2, det2 = y1 * x2;
	auto const det = det1 - det2;

	if (abs(det) > error_scale * (abs(det1) + abs(det2)))
		return det <=> 0;
	else
		return Exact(x1) * Exact(y2) - Exact(y1) * Exact(x2) <=> 0;
}

auto operator%(Edge const &edge1, Edge const &edge2) { // 3d cross product
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
