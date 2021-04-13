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

auto operator<(const Edge &edge1, const Edge &edge2) {
	return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
}

auto operator^(const Edge &edge1, const Edge &edge2) {
	const auto [x1, y1] = *edge1.second - *edge1.first;
	const auto [x2, y2] = *edge2.second - *edge2.first;
	return Exact(x1) * Exact(y2) - Exact(y1) * Exact(x2);
}

auto operator*(const Edge &edge1, const Edge &edge2) {
	const auto [x1, y1] = *edge1.second - *edge1.first;
	const auto [x2, y2] = *edge2.second - *edge2.first;
	return Exact(x1) * Exact(y2) + Exact(y1) * Exact(x2);
}

auto operator<=>(const Edge &edge1, const Edge &edge2) {
	using std::abs, IEEE754::epsilon;
	static constexpr auto error_scale = epsilon() * (1 + 2 * epsilon());

	const auto [x1, y1] = *edge1.second - *edge1.first;
	const auto [x2, y2] = *edge2.second - *edge2.first;
	const auto det1 = x1 * y2, det2 = y1 * x2;
	const auto det = det1 - det2;

	if (abs(det) > error_scale * (abs(det1) + abs(det2)))
		return det <=> 0;
	else
		return Exact(x1) * Exact(y2) <=> Exact(y1) * Exact(x2);
}

auto operator%(const Edge &edge1, const Edge &edge2) { // 3d cross product
	return (+*edge1.first - +*edge1.second) ^ (+*edge2.first - +*edge2.second);
}

auto operator>(const Edge &edge, double length) {
	return (*edge.second - *edge.first).sqnorm() > length * length;
}

auto operator-(const Edge &edge) {
	return Edge(edge.second, edge.first);
}

template <> struct std::hash<Edge> {
	std::size_t operator()(const Edge &edge) const {
		static constexpr auto hash = std::hash<PointIterator>();
		const auto seed = hash(edge.first);
		return seed ^ (hash(edge.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
