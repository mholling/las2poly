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
	if (det == 0) return false; // TODO: test vertex intersections, etc
	auto const t1 = ((u2 - u1) ^ (v2 - u2)) / det;
	auto const t2 = ((u2 - u1) ^ (v1 - u1)) / det;
	return t1 > 0 && t1 < 1 && t2 > 0 && t2 < 1;
}

// segment <  vector : vector lies to the right of segment
// segment <= vector : vector lies to the right of segment or is colinear
// segment >= vector : vector lies to the left of segment or is colinear
// segment >  vector : vector lies to the left of segment

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
