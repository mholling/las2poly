#ifndef EDGE_HPP
#define EDGE_HPP

#include "point.hpp"
#include <utility>
#include <functional>
#include <cstddef>

using Edge = std::pair<Point, Point>;

auto operator==(const Edge &edge1, const Edge &edge2) {
	return edge1.first == edge2.first && edge1.second == edge2.second;
}

auto operator||(const Edge &edge1, const Edge &edge2) {
	return edge1.first == edge2.second && edge1.second == edge2.first;
}

auto operator<(const Edge &edge1, const Edge &edge2) {
	return (edge1.second - edge1.first).sqnorm() < (edge2.second - edge2.first).sqnorm();
}

auto operator^(const Edge &edge1, const Edge &edge2) {
	return (edge1.second - edge1.first) ^ (edge2.second - edge2.first);
}

auto operator*(const Edge &edge1, const Edge &edge2) {
	return (edge1.second - edge1.first) * (edge2.second - edge2.first);
}

auto operator%(const Edge &edge1, const Edge &edge2) { // 3d cross product
	return (+edge1.first - +edge1.second) ^ (+edge1.first - +edge1.second);
}

auto operator&&(const Edge &edge, const Point &p) {
	return edge.first == p || edge.second == p;
}

auto operator<<(const Edge &edge, const Point &p) {
	return (edge.first < p) && !(edge.second < p) && ((edge.first - p) ^ (edge.second - p)) > 0;
}

auto operator>>(const Edge &edge, const Point &p) {
	return (edge.second < p) && !(edge.first < p) && ((edge.second - p) ^ (edge.first - p)) > 0;
}

auto operator^(const Edge &edge, const Point &p) {
	return (edge.first - p) ^ (edge.second - p);
}

auto operator>(const Edge &edge, double length) {
	return (edge.second - edge.first).sqnorm() > length * length;
}

auto operator-(const Edge &edge) {
	return Edge(edge.second, edge.first);
}

template <> struct std::hash<Edge> {
	std::size_t operator()(const Edge &edge) const {
		return hash<Point>()(edge.first) << 32 | hash<Point>()(edge.second) & 0xFFFFFFFF;
	}
};

#endif
