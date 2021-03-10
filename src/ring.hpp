#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include <vector>
#include <stdexcept>
#include <utility>
#include <cmath>
#include <algorithm>
#include <ostream>

class Ring {
	using Vertex = Vector<2>;
	using Vertices = std::vector<Vertex>;
	using VertexIterator = typename Vertices::const_iterator;

	Vertices vertices;
	double signed_area;

	struct VertexOnRing : std::runtime_error {
		VertexOnRing() : runtime_error("vertex on ring") { }
	};

	struct Iterator {
		const VertexIterator start, stop;
		VertexIterator here;

		Iterator(VertexIterator start, VertexIterator stop, VertexIterator here) : start(start), stop(stop), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto operator!=(Iterator other) const { return here != other.here; }
		auto operator*() { return std::pair(*here, *(here + 1 == stop ? start : here + 1)); }
	};

	auto begin() const { return Iterator(vertices.begin(), vertices.end(), vertices.begin()); }
	auto   end() const { return Iterator(vertices.begin(), vertices.end(), vertices.end()); }

	auto winding_number(const Vertex &v) const {
		int winding = 0;
		for (const auto &[v1, v2]: *this)
			if (v1 == v || v2 == v)
				throw VertexOnRing();
			else if ((v1 < v) && !(v2 < v) && ((v1 - v) ^ (v2 - v)) > 0)
				++winding;
			else if ((v2 < v) && !(v1 < v) && ((v2 - v) ^ (v1 - v)) > 0)
				--winding;
		return winding;
	}

	auto update_signed_area() {
		const auto &v = vertices[0];
		double sum = 0.0;
		for (const auto &[v0, v1, v2]: *this)
			sum += (v1 - v) ^ (v2 - v);
		signed_area = sum * 0.5;
	}

public:
	template <typename Edges>
	Ring(const Edges &edges) {
		for (const auto &[p1, p2]: edges)
			vertices.push_back(p1);
		update_signed_area();
	}

	auto contains(const Ring &ring) const {
		for (const auto &vertex: vertices)
			try { return winding_number(vertex) != 0; }
			catch (VertexOnRing &) { }
		return false; // ring == *this
	}

	friend auto operator<(const Ring &ring1, const Ring &ring2) {
		return ring1.signed_area < ring2.signed_area;
	}

	friend auto operator<(const Ring &ring, double signed_area) {
		return ring.signed_area < signed_area;
	}

	friend auto operator>(const Ring &ring, double signed_area) {
		return ring.signed_area > signed_area;
	}

	friend auto &operator<<(std::ostream &json, const Ring &ring) {
		json << '[';
		for (const auto &vertex: ring.vertices)
			json << vertex << ',';
		return json << ring.vertices[0] << ']';
	}
};

#endif
