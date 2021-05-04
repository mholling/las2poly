////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include "vector.hpp"
#include <utility>
#include <compare>

using Segment = std::pair<Vector<2>, Vector<2>>;

auto operator&&(Segment const &s1, Segment const &s2) {
	auto const &[u1, v1] = s1;
	auto const &[u2, v2] = s2;
	auto const det = (v1 - u1) ^ (v2 - u2);
	if (det == 0)
		return false;
	auto const t1 = (u2 - u1) ^ (v2 - u2);
	auto const t2 = (u2 - u1) ^ (v1 - u1);
	if (det > 0)
		return t1 > 0 && t1 < det && t2 > 0 && t2 < det;
	else
		return t1 < 0 && t1 > det && t2 < 0 && t2 > det;
}

// segment <  vector : segment lies to the left of vector
// segment <= vector : segment lies to the left of vector or is colinear
// segment >= vector : segment lies to the right of vector or is colinear
// segment >  vector : segment lies to the right of vector

auto operator<=>(Segment const &segment, Vector<2> const &vector) {
	auto const &[v1, v2] = segment;
	auto const &[x1, y1] = v1;
	auto const &[x2, y2] = v2;
	auto const &[x3, y3] = vector;

	auto const det1 = (x2 - x1) * (y3 - y2);
	auto const det2 = (x3 - x2) * (y2 - y1);
	auto const det = det1 - det2;

	return det <=> 0;
}

#endif
