#ifndef EXTERIOR_HPP
#define EXTERIOR_HPP

#include "point.hpp"
#include "edge.hpp"
#include <unordered_map>
#include <iterator>
#include <utility>
#include <algorithm>

template <typename Edges, typename Neighbours>
class Exterior {
	const Neighbours &neighbours;
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
		auto &operator++() { edge = exterior.next(edge); return *this; }
		auto &operator--() { edge = exterior.prev(edge); return *this; }
		auto &operator++(int) { auto old = *this; ++(*this); return old; }
		auto &operator--(int) { auto old = *this; --(*this); return old; }
		auto &operator*() { return edge; }
		auto operator->() { return &edge; }
	};

	auto &operator-=(const Edge &edge) {
		auto prev = before.find(edge)->second;
		auto next = after.find(edge)->second;
		after.erase(edge);
		before.erase(edge);
		auto p = neighbours.find(edge)->second.vertices;
		p.rotate(p.begin(), std::find(p.begin(), p.end(), edge.p1), p.end());
		auto edge1 = Edge(p[2], p[1]);
		auto edge2 = Edge(p[1], p[0]);
		after.insert(std::make_pair(prev, edge1));
		after.insert(std::make_pair(edge1, edge2));
		after.insert(std::make_pair(edge2, next));
		before.insert(std::make_pair(next, edge2));
		before.insert(std::make_pair(edge2, edge1));
		before.insert(std::make_pair(edge1, prev));
		return *this;
	}

public:
	Exterior(const Edges &edges, const Neighbours &neighbours) : neighbours(neighbours) {
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
