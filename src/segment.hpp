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

#endif
