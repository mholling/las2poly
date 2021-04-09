#ifndef RING_HPP
#define RING_HPP

#include "vector.hpp"
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
		const Ring &ring;
		VertexIterator here;

		CornerIterator(const Ring &ring, VertexIterator here) : ring(ring), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto &operator--() { --here; return *this;}
		auto operator!=(const CornerIterator &other) const { return here != other.here; }
		auto next() const { return *this != --ring.end() ? ++CornerIterator(ring, here) : ring.begin(); }
		auto prev() const { return *this != ring.begin() ? --CornerIterator(ring, here) : --ring.end(); }
		operator VertexIterator() const { return here; }
		operator const Vertex &() const { return *here; }
		auto operator*() const { return std::tuple<Vertex, Vertex, Vertex>(prev(), *here, next()); }
		auto cross() const { return (*here - prev()) ^ (next() - *here); }
		auto cosine() const { return (*here - prev()).normalise() * (next() - *here).normalise(); }
	};

	const Vertices &vertices() const {
		return *this;
	}

	auto winding_number(const Vertex &v) const {
		auto winding = 0;
		for (const auto [v0, v1, v2]: *this)
			if (v1 == v || v2 == v)
				throw VertexOnRing();
			else if ((v1 < v) && !(v2 < v) && ((v1 - v) ^ (v2 - v)) > 0)
				++winding;
			else if ((v2 < v) && !(v1 < v) && ((v2 - v) ^ (v1 - v)) > 0)
				--winding;
		return winding;
	}

	template <bool erode>
	struct CompareCornerAreas {
		auto operator()(const CornerIterator &v) const {
			const auto cross = v.cross();
			return std::pair(erode == cross < 0, std::abs(cross));
		}

		auto operator()(double corner_area) const {
			return std::pair(false, 2 * corner_area);
		}

		auto operator()(const CornerIterator &u, const CornerIterator &v) const {
			return (*this)(u) < (*this)(v);
		}
	};

	struct CompareCornerAngles {
		auto operator()(const CornerIterator &u, const CornerIterator &v) const {
			return u.cosine() < v.cosine();
		}
	};

	template <bool erode>
	void simplify(double tolerance) {
		using Compare = CompareCornerAreas<erode>;
		const auto compare = Compare();
		const auto limit = compare(tolerance);
		auto corners = std::multiset<CornerIterator, Compare>();
		for (auto corner = begin(); corner != end(); ++corner)
			corners.insert(corner);
		while (corners.size() > 4 && compare(*corners.begin()) < limit) {
			const auto corner = corners.begin();
			const auto vertex = *corner;
			const auto prev = vertex.prev();
			const auto next = vertex.next();
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
	Ring(const Edges &edges) : signed_area(0) {
		const auto &p = edges.begin()->first;
		for (const auto &[p1, p2]: edges) {
			push_back(*p1);
			signed_area += (*p1 - *p) ^ (*p2 - *p);
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
		simplify<true>(tolerance);
		simplify<false>(tolerance);
	}

	void smooth(double tolerance, double angle) {
		auto corners = std::multiset<CornerIterator, CompareCornerAngles>();
		for (auto corner = begin(); corner != end(); ++corner)
			corners.insert(corner);
		for (const auto cosine = std::cos(angle); corners.begin()->cosine() < cosine; ) {
			const auto corner = corners.begin();
			const auto vertex = *corner;
			const auto prev = vertex.prev();
			const auto next = vertex.next();
			const auto [v0, v1, v2] = **corner;
			const auto f0 = std::min(0.25, tolerance / (v1 - v0).norm());
			const auto f2 = std::min(0.25, tolerance / (v2 - v1).norm());
			const auto v10 = v0 * f0 + v1 * (1.0 - f0);
			const auto v11 = v2 * f2 + v1 * (1.0 - f2);
			corners.erase(corner);
			insert(insert(erase(vertex), v11), v10);
			corners.insert(next.prev());
			corners.insert(prev.next());
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
