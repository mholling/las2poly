////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include "summation.hpp"
#include <list>
#include <tuple>
#include <utility>
#include <algorithm>
#include <compare>

struct Ring : std::list<Vector<2>> {
	using Vertex = Vector<2>;
	using Vertices = std::list<Vertex>;
	using VertexIterator = Vertices::const_iterator;

	template <typename Ring>
	struct Iterator {
		Ring *ring;
		VertexIterator here;

		Iterator(Ring *ring, VertexIterator here) : ring(ring), here(here) { }

		auto &operator++() {
			++here;
			return *this;
		}

		auto &operator--() {
			--here;
			return *this;
		}

		auto operator!=(Iterator const &other) const {
			return ring != other.ring || here != other.here;
		}

		auto operator==(Iterator const &other) const {
			return ring == other.ring && here == other.here;
		}

		auto next() const {
			return here != --ring->end() ? ++Iterator(ring, here) : Iterator(ring, ring->begin());
		}

		auto prev() const {
			return here != ring->begin() ? --Iterator(ring, here) : --Iterator(ring, ring->end());
		}

		auto operator*() const {
			return std::tuple(*prev().here, *here, *next().here);
		}

		auto cross() const {
			auto const [v0, v1, v2] = **this;
			return (v1 - v0) ^ (v2 - v1);
		}

		auto cosine() const {
			auto const [v0, v1, v2] = **this;
			return (v1 - v0).normalise() * (v2 - v1).normalise();
		}

		auto erase() const {
			return ring->erase(here);
		}

		auto replace(Vertex const &v1, Vertex const &v2) const {
			auto const i3 = ring->erase(here);
			auto const i2 = ring->insert(i3, v2);
			auto const i1 = ring->insert(i2, v1);
			return std::pair(Iterator(ring, i1), Iterator(ring, i2));
		}

		auto ring_size() const {
			return ring->size();
		}
	};

	using CornerIterator = Iterator<Ring>;

	auto corners_begin() { return Iterator(this, begin()); }
	auto   corners_end() { return Iterator(this, end()); }

	struct Corners {
		Ring const *ring;
		Corners(Ring const *ring) : ring(ring) { }
		auto begin() const { return Iterator(ring, ring->begin()); }
		auto   end() const { return Iterator(ring, ring->end()); }
	};

	template <typename Edges>
	Ring(Edges const &edges) {
		for (auto const &[p1, p2]: edges)
			push_back(*p1);
	}

	auto anticlockwise() const {
		auto const leftmost = std::min_element(begin(), end());
		return Iterator(this, leftmost).cross() > 0;
	}

	auto signed_area(bool ogc) const {
		auto cross_product_sum = 0.0;
		auto const v = *begin();
		for (auto summation = Summation(cross_product_sum); auto const [v0, v1, v2]: Corners(this))
			summation += (v1 - v) ^ (v2 - v);
		return cross_product_sum * (ogc ? 0.5 : -0.5);
	}

	// ring <=> vertex  < 0 : vertex inside clockwise ring
	// ring <=> vertex == 0 : vertex on or outside ring
	// ring <=> vertex  > 0 : vertex inside anticlockwise ring

	friend auto operator<=>(Ring const &ring, Vertex const &v) {
		auto winding = 0;
		for (auto const [v0, v1, v2]: Corners(&ring))
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
