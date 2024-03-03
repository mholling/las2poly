////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include "corners.hpp"
#include "corner.hpp"
#include "summation.hpp"
#include <list>
#include <algorithm>
#include <compare>

class Ring : public std::list<Vector<2>> {
	using Vertex = Vector<2>;

	bool ogc;

public:
	template <typename Edges>
	Ring(Edges const &edges, bool ogc) : ogc(ogc) {
		for (auto const &[p1, p2]: edges)
			push_back(*p1);
	}

	auto corners() {
		return Corners(*this);
	}

	auto corners() const {
		return Corners(*this);
	}

	auto anticlockwise() const {
		auto const leftmost = std::min_element(begin(), end());
		return Corner(this, leftmost).cross() > 0;
	}

	auto exterior() const {
		return anticlockwise() == ogc;
	}

	auto signed_area() const {
		auto cross_product_sum = 0.0;
		auto const v = *begin();
		for (auto summation = Summation(cross_product_sum); auto const &[v0, v1, v2]: corners())
			summation += (v1 - v) ^ (v2 - v);
		return cross_product_sum * (ogc ? 0.5 : -0.5);
	}

	// ring <=> vertex  < 0 : vertex inside clockwise ring
	// ring <=> vertex == 0 : vertex on or outside ring
	// ring <=> vertex  > 0 : vertex inside anticlockwise ring

	friend auto operator<=>(Ring const &ring, Vertex const &v) {
		auto winding = 0;
		for (auto const &[v0, v1, v2]: ring.corners())
			if (v1 == v)
				return 0 <=> 0;
			else if ((v1 < v) && !(v2 < v) && ((v1 - v) ^ (v2 - v)) > 0)
				++winding;
			else if ((v2 < v) && !(v1 < v) && ((v2 - v) ^ (v1 - v)) > 0)
				--winding;
		return winding <=> 0;
	}

	// ring1 <=> ring2  < 0 : ring1 is clockwise and contains ring2
	// ring1 <=> ring2 == 0 : ring1 and ring2 are disjoint or the same
	// ring1 <=> ring2  > 0 : ring1 is anticlockwise and contains ring2

	friend auto operator<=>(Ring const &ring1, Ring const &ring2) {
		for (auto const &vertex: ring2)
			if (auto const result = ring1 <=> vertex; !(result == 0))
				return result;
		return 0 <=> 0;
	}
};

#endif
