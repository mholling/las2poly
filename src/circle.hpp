#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "points.hpp"
#include "ieee754.hpp"
#include "edge.hpp"
#include <tuple>
#include <cmath>
#include <compare>

using Circle = std::tuple<PointIterator, PointIterator, PointIterator>;

// circle <  point : point is outside circle
// circle == point : point is on boundary
// circle >  point : point is inside circle

auto operator<=>(const Circle &circle, const PointIterator &point) {
	using std::abs, IEEE754::epsilon;
	static constexpr auto error_scale = epsilon() * (10 + 96 * epsilon());

	const auto &[a, b, c] = circle;
	const auto [ax, ay] = *a - *point;
	const auto [bx, by] = *b - *point;
	const auto [cx, cy] = *c - *point;
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
		return det <=> 0;
	else {
		const auto ea = Edge(point, a);
		const auto eb = Edge(point, b);
		const auto ec = Edge(point, c);
		return (ea * ea) * (eb ^ ec) + (eb * eb) * (ec ^ ea) + (ec * ec) * (ea ^ eb) <=> 0;
	}
}

#endif
