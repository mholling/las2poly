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
#include <stdexcept>
#include <tuple>
#include <cmath>
#include <utility>
#include <set>
#include <algorithm>
#include <ostream>

class Ring : std::list<Vector<2>> {
	using Vertex = Vector<2>;
	using Vertices = std::list<Vertex>;
	using VertexIterator = typename Vertices::const_iterator;

	double signed_area;

	struct VertexOnRing : std::runtime_error {
		VertexOnRing() : runtime_error("vertex on ring") { }
	};

	auto begin() const { return CornerIterator(*this, Vertices::begin()); }
	auto   end() const { return CornerIterator(*this, Vertices::end()); }

	struct CornerIterator {
		Ring const &ring;
		VertexIterator here;

		CornerIterator(Ring const &ring, VertexIterator here) : ring(ring), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto &operator--() { --here; return *this;}
		auto operator!=(CornerIterator const &other) const { return here != other.here; }
		auto next() const { return *this != --ring.end() ? ++CornerIterator(ring, here) : ring.begin(); }
		auto prev() const { return *this != ring.begin() ? --CornerIterator(ring, here) : --ring.end(); }
		operator VertexIterator() const { return here; }
		auto operator*() const { return std::tuple<Vertex, Vertex, Vertex>(*prev().here, *here, *next().here); }
		auto  cross() const { auto const [v0, v1, v2] = **this; return (v1 - v0) ^ (v2 - v1); }
		auto cosine() const { auto const [v0, v1, v2] = **this; return (v1 - v0).normalise() * (v2 - v1).normalise(); }
	};

	Vertices const &vertices() const {
		return *this;
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

	struct CompareCornerAreas {
		bool erode;

		CompareCornerAreas(bool erode) : erode(erode) { }

		auto operator()(CornerIterator const &v) const {
			auto const cross = v.cross();
			return std::pair(erode == (cross < 0), std::abs(cross));
		}

		auto operator()(double corner_area) const {
			return std::pair(false, 2 * corner_area);
		}

		auto operator()(CornerIterator const &u, CornerIterator const &v) const {
			return (*this)(u) < (*this)(v);
		}
	};

	struct CompareCornerAngles {
		auto operator()(CornerIterator const &u, CornerIterator const &v) const {
			return u.cosine() < v.cosine();
		}
	};

	void simplify_one_sided(double tolerance, bool erode) {
		auto const compare = CompareCornerAreas(erode);
		auto const limit = compare(tolerance);
		auto corners = std::multiset<CornerIterator, CompareCornerAreas>(compare);
		for (auto corner = begin(); corner != end(); ++corner)
			corners.insert(corner);
		while (corners.size() > 4 && compare(*corners.begin()) < limit) {
			auto const corner = corners.begin();
			auto const vertex = *corner;
			auto const prev = vertex.prev();
			auto const next = vertex.next();
			corners.erase(corner);
			corners.erase(prev);
			corners.erase(next);
			erase(vertex);
			corners.insert(next.prev());
			corners.insert(prev.next());
		}
	}

public:
	template <typename Edges>
	Ring(Edges const &edges) : signed_area(0) {
		auto const &p = edges.begin()->first;
		for (auto sum = Summation(signed_area); auto const &[p1, p2]: edges) {
			push_back(*p1);
			sum += (*p1 - *p) ^ (*p2 - *p);
		}
		signed_area /= 2;
	}

	auto contains(Ring const &ring) const {
		for (auto const &vertex: ring.vertices())
			try { return winding_number(vertex) != 0; }
			catch (VertexOnRing &) { }
		return false; // ring == *this
	}

	void simplify(double tolerance, bool open) {
		simplify_one_sided(tolerance, !open);
		simplify_one_sided(tolerance, open);
	}

	void smooth(double tolerance, double angle) {
		auto corners = std::multiset<CornerIterator, CompareCornerAngles>();
		for (auto corner = begin(); corner != end(); ++corner)
			corners.insert(corner);
		for (auto const cosine = std::cos(angle); corners.begin()->cosine() < cosine; ) {
			auto const corner = corners.begin();
			auto const vertex = *corner;
			auto const prev = vertex.prev();
			auto const next = vertex.next();
			auto const [v0, v1, v2] = **corner;
			auto const f0 = std::min(0.25, tolerance / (v1 - v0).norm());
			auto const f2 = std::min(0.25, tolerance / (v2 - v1).norm());
			auto const v10 = v0 * f0 + v1 * (1.0 - f0);
			auto const v11 = v2 * f2 + v1 * (1.0 - f2);
			corners.erase(corner);
			insert(insert(erase(vertex), v11), v10);
			corners.insert(next.prev());
			corners.insert(prev.next());
		}
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
