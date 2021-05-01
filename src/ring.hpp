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
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <ostream>

class Ring : std::list<Vector<2>> {
	using Vertex = Vector<2>;
	using Vertices = std::list<Vertex>;
	using VertexIterator = typename Vertices::const_iterator;

	double signed_area;

	struct VertexOnRing : std::runtime_error {
		VertexOnRing() : runtime_error("vertex on ring") { }
	};

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
			return here != other.here;
		}

		auto operator==(Iterator const &other) const {
			return here == other.here;
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

		auto remove() const {
			return ring->erase(here);
		}

		auto replace(Vertex const &v1, Vertex const &v2) const {
			return ring->insert(ring->insert(remove(), v2), v1);
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
	Ring(Edges const &edges) : signed_area(0) {
		auto const &p = edges.begin()->first;
		for (auto sum = Summation(signed_area); auto const &[p1, p2]: edges) {
			push_back(*p1);
			sum += (*p1 - *p) ^ (*p2 - *p);
		}
		signed_area /= 2;
	}

	auto winding_number(Vertex const &v) const {
		auto winding = 0;
		for (auto const [v0, v1, v2]: *this)
			if (v1 == v || v2 == v)
				throw VertexOnRing();
			else if ((v1 < v) && !(v2 < v) && ((v1 - v) ^ (v2 - v)) > 0)
				++winding;
			else if ((v2 < v) && !(v1 < v) && ((v2 - v) ^ (v1 - v)) > 0)
				--winding;
		return winding;
	}

	auto contains(Ring const &ring) const {
		for (auto const &vertex: ring.vertices())
			try { return winding_number(vertex) != 0; }
			catch (VertexOnRing &) { }
		return false; // ring == *this
	}

	friend auto operator<(Ring const &ring1, Ring const &ring2) {
		return ring1.signed_area < ring2.signed_area;
	}

	friend auto operator<(Ring const &ring, double signed_area) {
		return ring.signed_area < signed_area;
	}

	friend auto operator>(Ring const &ring, double signed_area) {
		return ring.signed_area > signed_area;
	}

	friend auto &operator<<(std::ostream &json, Ring const &ring) {
		json << '[';
		for (auto const &vertex: ring.vertices())
			json << vertex << ',';
		return json << ring.front() << ']';
	}
};

#endif
