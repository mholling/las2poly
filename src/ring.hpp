#ifndef RING_HPP
#define RING_HPP

#include "point.hpp"
#include "vertices.hpp"
#include "edge.hpp"
#include <stdexcept>
#include <numeric>
#include <vector>
#include <unordered_set>
#include <ostream>

class Ring : public Vertices<std::vector<const Point *>> {
	double signed_area;

	struct PointOnRing : std::runtime_error {
		PointOnRing() : runtime_error("point on ring") { }
	};

	auto winding_number(const Point &point) const {
		int winding = 0;
		for (const auto edge: edges())
			if (edge & point)
				throw PointOnRing();
			else if (edge << point)
				++winding;
			else if (edge >> point)
				--winding;
		return winding;
	}

	Ring(std::unordered_map<Edge, Edge, Edge::Hash> &connections) {
		Edge::follow(connections, [&](const auto &edge) {
			vertices.push_back(&edge.p0);
		});

		double sum = 0.0;
		const auto &origin = *begin();
		for (const auto edge: edges())
			sum += edge ^ origin;
		signed_area = sum * 0.5;
	}

public:
	template <typename C>
	static auto from_edges(const C &edges) {
		auto connections = Edge::connections_for<true>(edges);
		std::vector<Ring> rings;
		while (!connections.empty()) {
			std::unordered_set<Edge, Edge::Hash> edges;
			Edge::follow(connections, [&](const auto &edge) {
				edges.insert(edge);
			});
			auto connections = Edge::connections_for<false>(edges);
			while (!connections.empty())
				rings.push_back(Ring(connections));
		}
		return rings;
	}

	auto contains(const Ring &ring) const {
		for (const auto &vertex: ring)
			try { return winding_number(vertex) != 0; }
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
		for (const auto &vertex: ring)
			json << vertex << ',';
		return json << *ring.begin() << ']';
	}
};

#endif
