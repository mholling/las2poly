////////////////////////////////////////////////////////////////////////////////
// Copyright 2022 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef CORNER_HPP
#define CORNER_HPP

#include <type_traits>
#include <cstddef>
#include <tuple>

template <typename Ring>
struct Corner {
	using Vertex = typename Ring::value_type;
	using VertexIterator = std::conditional_t<std::is_const_v<Ring>, typename Ring::const_iterator, typename Ring::iterator>;

	Ring *ring;
	VertexIterator here;

	Corner(Ring *ring, VertexIterator here) :
		ring(ring),
		here(here)
	{ }

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

	auto &operator*() const {
		return *this;
	}

	auto &operator()() const {
		return *here;
	}

	template <std::size_t N>
	auto &get() const {
		if constexpr (N == 0) return prev()();
		if constexpr (N == 1) return (*this)();
		if constexpr (N == 2) return next()();
	}

	auto cross() const {
		auto const &[v0, v1, v2] = *this;
		return (v1 - v0) ^ (v2 - v1);
	}

	auto cosine() const {
		auto const &[v0, v1, v2] = *this;
		return (v1 - v0).normalise() * (v2 - v1).normalise();
	}

	void erase() const {
		ring->erase(here);
	}

	auto update(Vertex const &vertex) const {
		*here = vertex;
	}

	auto ring_size() const {
		return ring->size();
	}
};

template <typename Ring>
struct std::tuple_size<Corner<Ring>> : std::integral_constant<std::size_t, 3> { };

template <typename Ring, std::size_t N>
struct std::tuple_element<N, Corner<Ring>> { using type = typename Corner<Ring>::Vertex; };

#endif
