#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "points.hpp"
#include "ieee754.hpp"
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
	auto const &[x1, y1] = *p1;
	auto const &[x2, y2] = *p2;
	auto const &[x3, y3] = *p3;
	auto const &[x4, y4] = *point;

	auto const dx1 = x1 - x4, dy1 = y1 - y4;
	auto const dx2 = x2 - x4, dy2 = y2 - y4;
	auto const dx3 = x3 - x4, dy3 = y3 - y4;
	auto const dot1 = dx1 * dx1 + dy1 * dy1;
	auto const dot2 = dx2 * dx2 + dy2 * dy2;
	auto const dot3 = dx3 * dx3 + dy3 * dy3;
	auto const dx2dy3 = dx2 * dy3, dx3dy2 = dx3 * dy2;
	auto const dx3dy1 = dx3 * dy1, dx1dy3 = dx1 * dy3;
	auto const dx1dy2 = dx1 * dy2, dx2dy1 = dx2 * dy1;
	auto const det = (
		dot1 * (dx2dy3 - dx3dy2) +
		dot2 * (dx3dy1 - dx1dy3) +
		dot3 * (dx1dy2 - dx2dy1)
	);
	auto const error_bound = error_scale * (
		dot1 * (abs(dx2dy3) + abs(dx3dy2)) +
		dot2 * (abs(dx3dy1) + abs(dx1dy3)) +
		dot3 * (abs(dx1dy2) + abs(dx2dy1))
	);
	if (abs(det) > error_bound)
		return det <=> 0;
	else {
		auto const dx1 = Exact(x1) - Exact(x4);
		auto const dx2 = Exact(x2) - Exact(x4);
		auto const dx3 = Exact(x3) - Exact(x4);
		auto const dy1 = Exact(y1) - Exact(y4);
		auto const dy2 = Exact(y2) - Exact(y4);
		auto const dy3 = Exact(y3) - Exact(y4);
		auto const dot1 = dx1 * dx1 + dy1 * dy1;
		auto const dot2 = dx2 * dx2 + dy2 * dy2;
		auto const dot3 = dx3 * dx3 + dy3 * dy3;
		auto const dx2dy3 = dx2 * dy3, dx3dy2 = dx3 * dy2;
		auto const dx3dy1 = dx3 * dy1, dx1dy3 = dx1 * dy3;
		auto const dx1dy2 = dx1 * dy2, dx2dy1 = dx2 * dy1;
		auto const det = (
			dot1 * (dx2dy3 - dx3dy2) +
			dot2 * (dx3dy1 - dx1dy3) +
			dot3 * (dx1dy2 - dx2dy1)
		);
		return det <=> 0;
	}
}

#endif
