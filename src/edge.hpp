#ifndef EDGE_HPP
#define EDGE_HPP

#include "points.hpp"
#include "exact.hpp"
#include <utility>
#include <cmath>
#include <functional>
#include <cstddef>

using Edge = std::pair<PointIterator, PointIterator>;

auto operator<(const Edge &edge1, const Edge &edge2) {
	return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
}

auto operator^(const Edge &edge1, const Edge &edge2) {
	const auto v1 = *edge1.second - *edge1.first;
	const auto v2 = *edge2.second - *edge2.first;
	// return v1 ^ v2;
	return Exact(v1[0]) * Exact(v2[1]) + Exact(-v1[1]) * Exact(v2[0]);
}

auto operator*(const Edge &edge1, const Edge &edge2) {
	const auto v1 = *edge1.second - *edge1.first;
	const auto v2 = *edge2.second - *edge2.first;
	// return v1 * v2;
	return Exact(v1[0]) * Exact(v2[0]) + Exact(v1[1]) * Exact(v2[1]);
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
		constexpr auto hash = std::hash<PointIterator>();
		const auto seed = hash(edge.first);
		return seed ^ (hash(edge.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
