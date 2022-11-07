////////////////////////////////////////////////////////////////////////////////
// Copyright 2022 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef CORNERS_HPP
#define CORNERS_HPP

#include "corner.hpp"

template <typename Ring>
struct Corners {
	Ring &ring;
	Corners(Ring &ring) : ring(ring) { }

	auto begin() const { return Corner(&ring, ring.begin()); }
	auto   end() const { return Corner(&ring, ring.end()); }
};

#endif
