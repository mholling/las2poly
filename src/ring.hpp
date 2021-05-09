////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include "bounds.hpp"
#include "summation.hpp"
#include <list>
#include <type_traits>
#include <tuple>
#include <utility>
#include <algorithm>
#include <compare>
#include <ostream>

class Ring : std::list<Vector<2>> {
	using Vertex = Vector<2>;
	using Vertices = std::list<Vertex>;
	using VertexIterator = typename Vertices::const_iterator;

	Vertices const &vertices() const {
		return *this;
	}

	template <bool is_const, typename Ring = std::conditional_t<is_const, Ring const, Ring>>
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
			return *this != --ring->end() ? ++Iterator(ring, here) : ring->begin();
		}

		auto prev() const {
			return *this != ring->begin() ? --Iterator(ring, here) : --ring->end();
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

		auto bounds() const {
			auto const [v0, v1, v2] = **this;
			return Bounds(v0, v1, v2);
		}
	};

public:
	using ConstCornerIterator = Iterator<true>;
	using CornerIterator = Iterator<false>;

	auto begin() const { return ConstCornerIterator(this, Vertices::begin()); }
	auto   end() const { return ConstCornerIterator(this, Vertices::end()); }
	auto begin() { return CornerIterator(this, Vertices::begin()); }
	auto   end() { return CornerIterator(this, Vertices::end()); }

	template <typename Edges>
	Ring(Edges const &edges) {
		for (auto const &[p1, p2]: edges)
			push_back(*p1);
	}

	auto is_exterior() const {
		auto const leftmost = std::min_element(list::begin(), list::end());
		return ConstCornerIterator(this, leftmost).cross() > 0;
	}

	auto signed_area() const {
		auto cross_product_sum = 0.0;
		auto const v = *list::begin();
		for (auto summation = Summation(cross_product_sum); auto const [v0, v1, v2]: *this)
			summation += (v1 - v) ^ (v2 - v);
		return cross_product_sum * 0.5;
	}

	// ring <=> vertex  < 0 : vertex inside clockwise ring
	// ring <=> vertex == 0 : vertex on or outside ring
	// ring <=> vertex  > 0 : vertex inside anticlockwise ring

	friend auto operator<=>(Ring const &ring, Vertex const &v) {
		auto winding = 0;
		for (auto const [v0, v1, v2]: ring)
			if (v1 == v)
				return 0 <=> 0;
			else if ((v1 < v) && !(v2 < v) && ((v1 - v) ^ (v2 - v)) > 0)
				++winding;
			else if ((v2 < v) && !(v1 < v) && ((v2 - v) ^ (v1 - v)) > 0)
				--winding;
		return winding <=> 0;
	}

	// ring1 <=> ring2  < 0 : ring1 contained by ring2
	// ring1 <=> ring2 == 0 : ring1 and ring2 are disjoint or the same
	// ring1 <=> ring2  > 0 : ring1 contains ring2

	friend auto operator<=>(Ring const &ring1, Ring const &ring2) {
		for (auto const &vertex: ring2.vertices())
			if (auto const result = ring1 <=> vertex; !(result == 0))
				return result;
		return 0 <=> 0;
	}

	friend auto &operator<<(std::ostream &json, Ring const &ring) {
		json << '[';
		for (auto const &vertex: ring.vertices())
			json << vertex << ',';
		return json << ring.front() << ']';
	}
};

#endif
