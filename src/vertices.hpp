#ifndef VERTICES_HPP
#define VERTICES_HPP

#include "point.hpp"
#include "edge.hpp"
#include <cstddef>

template <typename C>
class Vertices {
	using I = typename C::const_iterator;

	class Edges {
		const I start, stop;

		struct Iterator {
			const I start, stop;
			I here;

			Iterator(I start, I stop, I here) : start(start), stop(stop), here(here) { }
			auto &operator++() { ++here; return *this;}
			auto operator++(int) { auto old = *this; ++here; return old;}
			auto operator==(Iterator other) const { return here == other.here; }
			auto operator!=(Iterator other) const { return here != other.here; }
			auto operator*() { return Edge(*here, *(here + 1 == stop ? start : here + 1)); }
		};

	public:
		Edges(I start, I stop) : start(start), stop(stop) { }
		auto begin() const { return Iterator(start, stop, start); }
		auto   end() const { return Iterator(start, stop, stop); }
	};

protected:
	C vertices;

	Vertices() { }
	Vertices(C vertices) : vertices(vertices) { }

public:
	auto begin() { return vertices.begin(); }
	auto   end() { return vertices.end(); }
	auto begin() const { return vertices.begin(); }
	auto   end() const { return vertices.end(); }
	auto edges() const { return Edges(begin(), end()); }

	auto &operator[](std::size_t n) const { return vertices[n]; }
};

#endif
