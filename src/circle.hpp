#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "points.hpp"
#include "ieee754.hpp"
#include "edge.hpp"
#include <cmath>

struct Circle {
	PointIterator a, b, c;

	Circle(const PointIterator &a, const PointIterator &b, const PointIterator &c) : a(a), b(b), c(c) { }

	auto contains(const PointIterator &p) const {
		using std::abs, IEEE754::epsilon;
		// TODO: add correct error scaling:
		constexpr auto error_scale = epsilon() * (10 + 96 * epsilon());

		const auto [ax, ay] = *a - *p;
		const auto [bx, by] = *b - *p;
		const auto [cx, cy] = *c - *p;
		const auto aa = ax * ax + ay * ay;
		const auto bb = bx * bx + by * by;
		const auto cc = cx * cx + cy * cy;
		const auto bxcy = bx * cy, bycx = by * cx;
		const auto cxay = cx * ay, cyax = cy * ax;
		const auto axby = ax * by, aybx = ay * bx;
		const auto det = aa * (bxcy - bycx) + bb * (cxay - cyax) + cc * (axby - aybx);

		const auto error_bound = error_scale * (
			aa * (abs(bxcy) + abs(bycx)) +
			bb * (abs(cxay) + abs(cyax)) +
			cc * (abs(axby) + abs(aybx))
		);

		if (abs(det) > error_bound)
			return det > 0;
		else {
			const auto ea = Edge(p, a);
			const auto eb = Edge(p, b);
			const auto ec = Edge(p, c);
			return (ea * ea) * (eb ^ ec) + (eb * eb) * (ec ^ ea) + (ec * ec) * (ea ^ eb) > 0;
		}
	}
};

#endif
