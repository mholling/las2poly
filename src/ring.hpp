#ifndef RING_HPP
#define RING_HPP

#include "point.hpp"
#include "edge.hpp"
#include <vector>
#include <stdexcept>
#include <utility>
#include <numeric>
#include <unordered_set>
#include <ostream>

class Ring {
	std::vector<Point> points;
	double signed_area;

	struct PointOnRing : std::runtime_error {
		PointOnRing() : runtime_error("point on ring") { }
	};

	template <typename Iterator>
	struct EdgeIterator {
		const Iterator start, stop;
		Iterator here;

		EdgeIterator(Iterator start, Iterator stop, Iterator here) : start(start), stop(stop), here(here) { }
		auto &operator++() { ++here; return *this;}
		// auto operator++(int) { auto old = *this; ++here; return old;}
		// auto operator==(EdgeIterator other) const { return here == other.here; }
		auto operator!=(EdgeIterator other) const { return here != other.here; }
		auto operator*() { return Edge(*here, *(here + 1 == stop ? start : here + 1)); }
	};

	auto begin() const { return EdgeIterator(points.begin(), points.end(), points.begin()); }
	auto   end() const { return EdgeIterator(points.begin(), points.end(), points.end()); }

	auto winding_number(const Point &point) const {
		int winding = 0;
		for (const auto edge: *this)
			if (edge && point)
				throw PointOnRing();
			else if (edge << point)
				++winding;
			else if (edge >> point)
				--winding;
		return winding;
	}

public:
	template <typename Edges>
	Ring(const Edges &edges) {
		const auto &origin = edges.begin()->first;
		double sum = 0.0;
		for (const auto &edge: edges) {
			points.push_back(edge.first);
			sum += edge ^ origin;
		}
		signed_area = sum * 0.5;
	}

	auto contains(const Ring &ring) const {
		for (const auto &point: points)
			try { return winding_number(point) != 0; }
			catch (PointOnRing &) { }
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

	friend std::ostream &operator<<(std::ostream &json, const Ring &ring) {
		json << '[';
		for (const auto &point: ring.points)
			json << point << ',';
		return json << ring.points[0] << ']';
	}
};

#endif
