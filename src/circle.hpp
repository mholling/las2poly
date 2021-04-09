#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "points.hpp"

struct Circle {
	PointIterator a, b, c;

	Circle(const PointIterator &a, const PointIterator &b, const PointIterator &c) : a(a), b(b), c(c) { }

	auto contains(const PointIterator &p) const {
		const auto aa = *a - *p, bb = *b - *p, cc = *c - *p;
		return (aa * aa) * (bb ^ cc) + (bb * bb) * (cc ^ aa) + (cc * cc) * (aa ^ bb) > 0;
	}
};

#endif
