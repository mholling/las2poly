#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include <utility>
#include <list>
#include <stdexcept>
#include <cstddef>
#include <map>
#include <cmath>
#include <algorithm>
#include <ostream>

class Ring {
	using Vertex = Vector<2>;
	using Corner = std::tuple<Vertex, Vertex, Vertex>;
	using Vertices = std::list<Vertex>;
	using VertexIterator = typename Vertices::const_iterator;

	Vertices vertices;
	double signed_area;

	struct VertexOnRing : std::runtime_error {
		VertexOnRing() : runtime_error("vertex on ring") { }
	};

	struct Iterator {
		const Vertices &vertices;
		VertexIterator here;

		Iterator(const Vertices &vertices, VertexIterator here) : vertices(vertices), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto &operator--() { --here; return *this;}
		auto operator!=(Iterator other) const { return here != other.here; }
		auto next() const { return here == --vertices.end() ? Iterator(vertices, vertices.begin()) : ++Iterator(vertices, here); }
		auto prev() const { return here == vertices.begin() ? --Iterator(vertices, vertices.end()) : --Iterator(vertices, here); }
		operator VertexIterator() const { return here; }
		operator Vertex() const { return *here; }
		auto operator*() const { return Corner(prev(), *here, next()); }
	};

	struct CompareCornerAreas {
		auto operator()(const Corner &u, const Corner &v) const {
			const auto &[u0, u1, u2] = u;
			const auto &[v0, v1, v2] = v;
			return std::abs((u1 - u0) ^ (u2 - u1)) < std::abs((v1 - v0) ^ (v2 - v1));
		}
	};

	auto begin() const { return Iterator(vertices, vertices.begin()); }
	auto   end() const { return Iterator(vertices, vertices.end()); }

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
		const auto &v = vertices.front();
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
		std::map<Corner, Iterator, CompareCornerAreas> corners;
		for (auto iterator = begin(); iterator != end(); ++iterator)
			corners.emplace(*iterator, iterator);
		while (corners.size() > 3) {
			const auto &[this_corner, iterator] = *corners.begin();
			const auto &[v0, v1, v2] = this_corner;
			if (std::abs((v1 - v0) ^ (v2 - v1)) > 2 * tolerance)
				break;
			auto prev = iterator.prev();
			auto next = iterator.next();
			auto prev_corner = *prev;
			auto next_corner = *next;
			vertices.erase(iterator);
			corners.erase(this_corner);
			corners.erase(prev_corner);
			corners.erase(next_corner);
			auto new_next = prev.next();
			auto new_prev = next.prev();
			corners.emplace(*new_prev, new_prev);
			corners.emplace(*new_next, new_next);
		}
		update_signed_area();
	}

	void smooth(double tolerance, double angle) {
		auto cosine = std::cos(angle * 3.141592653589793 / 180.0);
		for (Vertices smoothed; smoothed.size() != vertices.size(); vertices.swap(smoothed)) {
			smoothed.clear();
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
		return json << ring.vertices.front() << ']';
	}
};

#endif
