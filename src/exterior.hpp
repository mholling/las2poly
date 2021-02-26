#ifndef EXTERIOR_HPP
#define EXTERIOR_HPP

#include "point.hpp"
#include "edge.hpp"
#include "ring.hpp"
#include <unordered_map>
#include <iterator>
#include <utility>
#include <algorithm>

template <typename Edges, bool anticlockwise = true>
class Exterior {
	std::unordered_map<Edge, Edge, Edge::Hash> connections;
	using Connection = decltype(connections)::const_iterator;

	auto follow(Connection connection) const {
		return connections.find(connection->second);
	}

	struct Iterator {
		const Exterior &exterior;
		Connection connection;

		Iterator(const Exterior &exterior, const Connection &connection) : exterior(exterior), connection(connection) { }
		auto &operator++() { connection = exterior.follow(connection); return *this;}
		auto &operator*() { return connection->second; }
		auto operator->() { return &connection->second; }
	};

public:
	Exterior(const Edges &edges) {
		std::unordered_map<Point, Edge, Point::Hash> points_edges;
		std::transform(edges.begin(), edges.end(), std::inserter(points_edges, points_edges.begin()), [](const auto &edge) {
			return std::make_pair(edge.p0, edge);
		});

		for (const auto &incoming: edges) {
			const auto &[point, outgoing] = *points_edges.find(incoming.p1);
			if constexpr (anticlockwise)
				connections.insert(std::make_pair(incoming, outgoing));
			else
				connections.insert(std::make_pair(-outgoing, -incoming));
		}
	}

	template <typename LessThan>
	auto begin(LessThan less_than) {
		auto connection = std::min_element(connections.begin(), connections.end(), [&](const auto &connection0, const auto &connection1) {
			return less_than(connection0.first.p1, connection1.first.p1) == anticlockwise;
		});
		return Iterator(*this, connection);
	}
};

#endif
