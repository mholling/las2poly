////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "points.hpp"
#include "exact.hpp"
#include <tuple>
#include <limits>
#include <cmath>
#include <compare>

using Circle = std::tuple<PointIterator, PointIterator, PointIterator>;

// circle <  point : point is outside circle
// circle <= point : point is outside or on boundary
// circle >= point : point is inside or on boundary
// circle >  point : point is inside circle

auto operator<=>(Circle const &circle, PointIterator const &point) {
	auto static constexpr epsilon = 0.5 * std::numeric_limits<double>::epsilon();
	auto static constexpr error_scale = epsilon * (10 + 96 * epsilon);

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
	auto const det1 = dot1 * (dx2dy3 - dx3dy2);
	auto const det2 = dot2 * (dx3dy1 - dx1dy3);
	auto const det3 = dot3 * (dx1dy2 - dx2dy1);
	auto const det = det1 + det2 + det3;

	auto const error_bound = error_scale * (
		dot1 * (std::abs(dx2dy3) + std::abs(dx3dy2)) +
		dot2 * (std::abs(dx3dy1) + std::abs(dx1dy3)) +
		dot3 * (std::abs(dx1dy2) + std::abs(dx2dy1))
	);

	if (abs(det) > error_bound)
		return det <=> 0;

	auto const [x_min, x_max] = std::minmax({x1, x2, x3, x4});
	auto const [y_min, y_max] = std::minmax({y1, y2, y3, y4});

	if ((2 * x_min > x_max || 2 * x_max < x_min) && (2 * y_min > y_max || 2 * y_max < y_min)) {
		auto const dx1 = Exact(x1 - x4), dy1 = Exact(y1 - y4);
		auto const dx2 = Exact(x2 - x4), dy2 = Exact(y2 - y4);
		auto const dx3 = Exact(x3 - x4), dy3 = Exact(y3 - y4);
		auto const det1 = (dx1 * dx1 + dy1 * dy1) * (dx2 * dy3 - dx3 * dy2);
		auto const det2 = (dx2 * dx2 + dy2 * dy2) * (dx3 * dy1 - dx1 * dy3);
		auto const det3 = (dx3 * dx3 + dy3 * dy3) * (dx1 * dy2 - dx2 * dy1);
		return det1 + det2 + det3 <=> 0;
	} else {
		auto const dx1 = Exact(x1) - Exact(x4), dy1 = Exact(y1) - Exact(y4);
		auto const dx2 = Exact(x2) - Exact(x4), dy2 = Exact(y2) - Exact(y4);
		auto const dx3 = Exact(x3) - Exact(x4), dy3 = Exact(y3) - Exact(y4);
		auto const det1 = (dx1 * dx1 + dy1 * dy1) * (dx2 * dy3 - dx3 * dy2);
		auto const det2 = (dx2 * dx2 + dy2 * dy2) * (dx3 * dy1 - dx1 * dy3);
		auto const det3 = (dx3 * dx3 + dy3 * dy3) * (dx1 * dy2 - dx2 * dy1);
		return det1 + det2 + det3 <=> 0;
	}
}

// // fastest version of worst-case test:
// auto const dot1 = Exact(x1) * Exact(x1) + Exact(y1) * Exact(y1);
// auto const dot2 = Exact(x2) * Exact(x2) + Exact(y2) * Exact(y2);
// auto const dot3 = Exact(x3) * Exact(x3) + Exact(y3) * Exact(y3);
// auto const dot4 = Exact(x4) * Exact(x4) + Exact(y4) * Exact(y4);
// auto const x1y2 = Exact(x1) * Exact(y2);
// auto const x1y3 = Exact(x1) * Exact(y3);
// auto const x1y4 = Exact(x1) * Exact(y4);
// auto const x2y1 = Exact(x2) * Exact(y1);
// auto const x2y3 = Exact(x2) * Exact(y3);
// auto const x2y4 = Exact(x2) * Exact(y4);
// auto const x3y1 = Exact(x3) * Exact(y1);
// auto const x3y2 = Exact(x3) * Exact(y2);
// auto const x3y4 = Exact(x3) * Exact(y4);
// auto const x4y1 = Exact(x4) * Exact(y1);
// auto const x4y2 = Exact(x4) * Exact(y2);
// auto const x4y3 = Exact(x4) * Exact(y3);
// auto const det1 = dot1 * (x2y3 - x2y4) + dot1 * (x3y4 - x3y2) + dot1 * (x4y2 - x4y3);
// auto const det2 = dot2 * (x3y1 - x3y4) + dot2 * (x4y3 - x4y1) + dot2 * (x1y4 - x1y3);
// auto const det3 = dot3 * (x4y1 - x4y2) + dot3 * (x1y2 - x1y4) + dot3 * (x2y4 - x2y1);
// auto const det4 = dot4 * (x1y3 - x1y2) + dot4 * (x2y1 - x2y3) + dot4 * (x3y2 - x3y1);
// return det1 + det2 + det3 + det4 <=> 0;

#endif
