#ifndef EDGE_HPP
#define EDGE_HPP

#include "point.hpp"
#include "points.hpp"
#include <utility>
#include <functional>
#include <cstddef>

using Edge = std::pair<PointIterator, PointIterator>;

auto operator<(const Edge &edge1, const Edge &edge2) {
	return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
}

auto operator^(const Edge &edge1, const Edge &edge2) {
	return (*edge1.second - *edge1.first) ^ (*edge2.second - *edge2.first);
}

auto operator*(const Edge &edge1, const Edge &edge2) {
	return (*edge1.second - *edge1.first) * (*edge2.second - *edge2.first);
}

auto operator%(const Edge &edge1, const Edge &edge2) { // 3d cross product
	return (+*edge1.first - +*edge1.second) ^ (+*edge1.first - +*edge1.second);
}

auto operator>(const Edge &edge, double length) {
	return (*edge.second - *edge.first).sqnorm() > length * length;
}

auto operator-(const Edge &edge) {
	return Edge(edge.second, edge.first);
}

template <> struct std::hash<Edge> {
	std::size_t operator()(const Edge &edge) const {
		auto constexpr hash = std::hash<PointIterator>();
		auto seed = hash(edge.first);
		return seed ^ (hash(edge.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
