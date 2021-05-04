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

// segment <  vector : segment lies to the left of vector
// segment <= vector : segment lies to the left of vector or is colinear
// segment >= vector : segment lies to the right of vector or is colinear
// segment >  vector : segment lies to the right of vector

auto operator<=>(Segment const &segment, Vector<2> const &vector) {
	auto const &[u, v] = segment;
	auto const det = (vector - u) ^ (vector - v);
	return det <=> 0;
}

auto operator&&(Segment const &s1, Segment const &s2) {
	auto const &[u1, v1] = s1;
	auto const &[u2, v2] = s2;
	return s1 <= u2 != s1 <= v2 && s2 <= u1 != s2 <= v1;
}

#endif
