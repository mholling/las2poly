#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include "corner.hpp"
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <iterator>
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
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = Corner;
		using reference         = Corner&;
		using pointer           = void;

		VertexIterator start, stop, here;

		Iterator(VertexIterator start, VertexIterator stop, VertexIterator here) : start(start), stop(stop), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto operator!=(Iterator other) const { return here != other.here; }
		auto operator*() { return Corner(*(here == start ? stop - 1 : here - 1), *here, *(here + 1 == stop ? start : here + 1)); }
		operator VertexIterator() const { return here; }
	};

	auto begin() const { return Iterator(vertices.begin(), vertices.end(), vertices.begin()); }
	auto   end() const { return Iterator(vertices.begin(), vertices.end(), vertices.end()); }

	auto winding_number(const Vertex &v) const {
		int winding = 0;
		for (const auto &[v0, v1, v2]: *this)
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

	void simplify(double tolerance) {
		while (vertices.size() > 3) {
			auto least_important = std::min_element(begin(), end());
			auto [v0, v1, v2] = *least_important;
			auto area = 0.5 * std::abs((v1 - v0) ^ (v2 - v1));
			if (area > tolerance)
				break;
			vertices.erase(least_important);
		}
		update_signed_area();
	}

	void smooth(double tolerance, double angle) {
		auto cosine = std::cos(angle * 3.141592653589793 / 180.0);
		while (true) {
			Vertices smoothed;
			for (const auto &[v0, v1, v2]: *this) {
				auto d0 = v1 - v0;
				auto d2 = v2 - v1;
				auto n0 = d0.norm();
				auto n2 = d2.norm();
				if (d0 * d2 > cosine * n0 * n2)
					smoothed.push_back(v1);
				else {
					auto f0 = std::min(0.25, tolerance / n0);
					auto f2 = std::min(0.25, tolerance / n2);
					smoothed.push_back(v0 * f0 + v1 * (1.0 - f0));
					smoothed.push_back(v2 * f2 + v1 * (1.0 - f2));
				}
			}
			if (smoothed.size() == vertices.size())
				break;
			vertices.swap(smoothed);
		}
		update_signed_area();
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
