////////////////////////////////////////////////////////////////////////////////
// Copyright 2022 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef CORNER_HPP
#define CORNER_HPP

#include <tuple>
#include <utility>

template <typename Ring>
struct Corner {
	using Vertex = typename Ring::value_type;
	using VertexIterator = typename Ring::const_iterator;

	Ring *ring;
	VertexIterator here;

	Corner(Ring *ring, VertexIterator here) : ring(ring), here(here) { }

	auto &operator++() {
		++here;
		return *this;
	}

	auto &operator--() {
		--here;
		return *this;
	}

	auto operator!=(Corner const &other) const {
		return ring != other.ring || here != other.here;
	}

	auto operator==(Corner const &other) const {
		return ring == other.ring && here == other.here;
	}

	auto next() const {
		return here != --ring->end() ? ++Corner(ring, here) : Corner(ring, ring->begin());
	}

	auto prev() const {
		return here != ring->begin() ? --Corner(ring, here) : --Corner(ring, ring->end());
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
		return std::pair(Corner(ring, i1), Corner(ring, i2));
	}

	auto ring_size() const {
		return ring->size();
	}
};

#endif
