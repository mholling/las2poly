#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "points.hpp"
#include "edge.hpp"

struct Circle {
	PointIterator a, b, c;

	Circle(const PointIterator &a, const PointIterator &b, const PointIterator &c) : a(a), b(b), c(c) { }

	auto contains(const PointIterator &p) const {
		const auto pa = Edge(p, a);
		const auto pb = Edge(p, b);
		const auto pc = Edge(p, c);
		return (pa * pa) * (pb ^ pc) + (pb * pb) * (pc ^ pa) + (pc * pc) * (pa ^ pb) > 0;
	}
};

#endif
