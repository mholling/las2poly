#ifndef EDGE_HPP
#define EDGE_HPP

#include "point.hpp"

struct Edge {
	const Point &p0, &p1;

	struct Hash {
		auto operator()(const Edge &edge) const { return Point::Hash()(edge.p0) ^ Point::Hash()(edge.p1); }
	};

	Edge(const Point &p0, const Point &p1) : p0(p0), p1(p1) { }

	friend auto operator==(const Edge &edge1, const Edge &edge2) {
		return edge1.p0 == edge2.p0 && edge1.p1 == edge2.p1;
	}

	friend auto operator||(const Edge &edge1, const Edge &edge2) {
		return edge1.p0 == edge2.p1 && edge1.p1 == edge2.p0;
	}

	auto operator&(const Point &p) const {
		return p0 == p || p1 == p;
	}

	auto operator<<(const Point &p) const {
		return (p0 < p) && !(p1 < p) && ((p0 - p) ^ (p1 - p)) > 0;
	}

	auto operator>>(const Point &p) const {
		return (p1 < p) && !(p0 < p) && ((p1 - p) ^ (p0 - p)) > 0;
	}

	auto operator^(const Point &p) const {
		return (p0 - p) ^ (p1 - p);
	}

	auto operator>(double length) const {
		return (p0 >> p1) > length;
	}

	auto operator-() const {
		return Edge(p1, p0);
	}

	auto delta() const {
		return p1 - p0;
	}
};

#endif
