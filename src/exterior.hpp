#ifndef EXTERIOR_HPP
#define EXTERIOR_HPP

#include "point.hpp"
#include "edge.hpp"
#include "ring.hpp"
#include <unordered_map>
#include <iterator>
#include <utility>
#include <algorithm>

template <typename Edges>
class Exterior {
	std::unordered_map<Edge, Edge, Edge::Hash> after, before;

	auto next(const Edge &edge) const {
		return after.find(edge)->second;
	}

	auto prev(const Edge &edge) const {
		return before.find(edge)->second;
	}

	struct Iterator {
		const Exterior &exterior;
		Edge edge;

		Iterator(const Exterior &exterior, const Edge &edge) : exterior(exterior), edge(edge) { }
		auto &operator++() { edge = exterior.next(edge); return *this;}
		auto &operator--() { edge = exterior.prev(edge); return *this;}
		auto &operator*() { return edge; }
		auto operator->() { return &edge; }
	};

public:
	Exterior(const Edges &edges) {
		std::unordered_map<Point, Edge, Point::Hash> points_edges;
		std::transform(edges.begin(), edges.end(), std::inserter(points_edges, points_edges.begin()), [](const auto &edge) {
			return std::make_pair(edge.p0, edge);
		});

		for (const auto &incoming: edges) {
			const auto &[point, outgoing] = *points_edges.find(incoming.p1);
			after.insert(std::make_pair(incoming, outgoing));
			before.insert(std::make_pair(outgoing, incoming));
		}
	}

	template <typename LessThan>
	auto leftmost(LessThan less_than) {
		auto &edge = std::min_element(after.begin(), after.end(), [&](const auto &connection0, const auto &connection1) {
			return less_than(connection0.first.p1, connection1.first.p1);
		})->second;
		return Iterator(*this, edge);
	}

	template <typename LessThan>
	auto rightmost(LessThan less_than) {
		auto &edge = std::max_element(after.begin(), after.end(), [&](const auto &connection0, const auto &connection1) {
			return less_than(connection0.first.p1, connection1.first.p1);
		})->first;
		return Iterator(*this, edge);
	}
};

#endif
