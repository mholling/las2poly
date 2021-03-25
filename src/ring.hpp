#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
#include "edge.hpp"
#include <list>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <set>
#include <algorithm>
#include <ostream>

class Ring : std::list<Vector<2>> {
	using Vertex = Vector<2>;
	using Vertices = std::list<Vertex>;
	using VertexIterator = typename Vertices::const_iterator;
	using Corner = std::tuple<const Vertex &, const Vertex &, const Vertex &>;

	double signed_area;

	struct VertexOnRing : std::runtime_error {
		VertexOnRing() : runtime_error("vertex on ring") { }
	};

	auto begin() const { return Iterator(*this, Vertices::begin()); }
	auto   end() const { return Iterator(*this, Vertices::end()); }

	struct Iterator {
		const Ring &ring;
		VertexIterator here;

		Iterator(const Ring &ring, VertexIterator here) : ring(ring), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto &operator--() { --here; return *this;}
		auto operator!=(const Iterator &other) const { return here != other.here; }
		auto next() const { return *this != --ring.end() ? ++Iterator(ring, here) : ring.begin(); }
		auto prev() const { return *this != ring.begin() ? --Iterator(ring, here) : --ring.end(); }
		operator VertexIterator() const { return here; }
		operator const Vertex &() const { return *here; }
		auto operator*() const { return Corner(prev(), *here, next()); }
	};

	struct CompareCornerAreas {
		auto operator()(const Iterator &u, const Iterator &v) const {
			const auto &[u0, u1, u2] = *u;
			const auto &[v0, v1, v2] = *v;
			return std::abs((u1 - u0) ^ (u2 - u1)) < std::abs((v1 - v0) ^ (v2 - v1));
		}
	};

	const Vertices &vertices() const {
		return *this;
	}

	auto winding_number(const Vertex &v) const {
		int winding = 0;
		for (const auto [v0, v1, v2]: *this)
			if (v1 == v || v2 == v)
				throw VertexOnRing();
			else if ((v1 < v) && !(v2 < v) && ((v1 - v) ^ (v2 - v)) > 0)
				++winding;
			else if ((v2 < v) && !(v1 < v) && ((v2 - v) ^ (v1 - v)) > 0)
				--winding;
		return winding;
	}

public:
	template <typename Edges>
	Ring(const Edges &edges) : signed_area(0) {
		const auto &p = edges.begin()->first;
		for (const auto &[p1, p2]: edges) {
			push_back(*p1);
			signed_area += Edge(p, p1) ^ Edge(p, p2);
		}
		signed_area /= 2;
	}

	auto contains(const Ring &ring) const {
		for (const auto &vertex: ring.vertices())
			try { return winding_number(vertex) != 0; }
			catch (VertexOnRing &) { }
		return false; // ring == *this
	}

	void simplify(double tolerance) {
		std::multiset<Iterator, CompareCornerAreas> corners;
		for (auto corner = begin(); corner != end(); ++corner)
			corners.insert(corner);
		while (corners.size() > 3) {
			const auto corner = *corners.begin();
			const auto &[v0, v1, v2] = *corner;
			if (std::abs((v1 - v0) ^ (v2 - v1)) > 2 * tolerance)
				break;
			auto prev = corner.prev();
			auto next = corner.next();
			corners.erase(corner);
			corners.erase(prev);
			corners.erase(next);
			erase(corner);
			corners.insert(next.prev());
			corners.insert(prev.next());
		}
	}

	void smooth(double tolerance, double angle) {
		auto cosine = std::cos(angle);
		for (Vertices smoothed; smoothed.size() != size(); swap(smoothed)) {
			smoothed.clear();
			for (const auto [v0, v1, v2]: *this) {
				auto d0 = v1 - v0;
				auto d2 = v2 - v1;
				auto n0 = d0.norm();
				auto n2 = d2.norm();
				if (d0 * d2 > n0 * n2 * cosine)
					smoothed.push_back(v1);
				else {
					double f0 = std::min(0.25, tolerance / n0);
					double f2 = std::min(0.25, tolerance / n2);
					smoothed.push_back(v0 * f0 + v1 * (1.0 - f0));
					smoothed.push_back(v2 * f2 + v1 * (1.0 - f2));
				}
			}
		}
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
		for (const auto &vertex: ring.vertices())
			json << vertex << ',';
		return json << ring.front() << ']';
	}
};

#endif
