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
// circle <= point : point is outside or on boundary
// circle >= point : point is inside or on boundary
// circle >  point : point is inside circle

auto operator<=>(Circle const &circle, PointIterator const &point) {
	using std::abs, IEEE754::epsilon;
	auto static constexpr error_scale = epsilon() * (10 + 96 * epsilon());

	auto const &[p1, p2, p3] = circle;
	auto const [x1, y1] = *p1 - *point;
	auto const [x2, y2] = *p2 - *point;
	auto const [x3, y3] = *p3 - *point;
	auto const dot1 = x1 * x1 + y1 * y1;
	auto const dot2 = x2 * x2 + y2 * y2;
	auto const dot3 = x3 * x3 + y3 * y3;
	auto const x2y3 = x2 * y3, y2x3 = y2 * x3;
	auto const x3y1 = x3 * y1, y3x1 = y3 * x1;
	auto const x1y2 = x1 * y2, y1x2 = y1 * x2;
	auto const det = dot1 * (x2y3 - y2x3) + dot2 * (x3y1 - y3x1) + dot3 * (x1y2 - y1x2);

	auto const error_bound = error_scale * (
		dot1 * (abs(x2y3) + abs(y2x3)) +
		dot2 * (abs(x3y1) + abs(y3x1)) +
		dot3 * (abs(x1y2) + abs(y1x2))
	);

	if (abs(det) > error_bound)
		return det <=> 0;
	else {
		auto const ea = Edge(point, p1);
		auto const eb = Edge(point, p2);
		auto const ec = Edge(point, p3);
		return (ea * ea) * (eb ^ ec) + (eb * eb) * (ec ^ ea) + (ec * ec) * (ea ^ eb) <=> 0;
	}
}

#endif
