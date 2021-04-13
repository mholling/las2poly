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

auto operator<=>(Circle const &circle, PointIterator const &point) {
	using std::abs, IEEE754::epsilon;
	auto static constexpr error_scale = epsilon() * (10 + 96 * epsilon());

	auto const &[a, b, c] = circle;
	auto const [ax, ay] = *a - *point;
	auto const [bx, by] = *b - *point;
	auto const [cx, cy] = *c - *point;
	auto const aa = ax * ax + ay * ay;
	auto const bb = bx * bx + by * by;
	auto const cc = cx * cx + cy * cy;
	auto const bxcy = bx * cy, bycx = by * cx;
	auto const cxay = cx * ay, cyax = cy * ax;
	auto const axby = ax * by, aybx = ay * bx;
	auto const det = aa * (bxcy - bycx) + bb * (cxay - cyax) + cc * (axby - aybx);

	auto const error_bound = error_scale * (
		aa * (abs(bxcy) + abs(bycx)) +
		bb * (abs(cxay) + abs(cyax)) +
		cc * (abs(axby) + abs(aybx))
	);

	if (abs(det) > error_bound)
		return det <=> 0;
	else {
		auto const ea = Edge(point, a);
		auto const eb = Edge(point, b);
		auto const ec = Edge(point, c);
		return (ea * ea) * (eb ^ ec) + (eb * eb) * (ec ^ ea) + (ec * ec) * (ea ^ eb) <=> 0;
	}
}

#endif
