#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP

#include "point.hpp"
#include "edge.hpp"
#include "ring.hpp"
#include <unordered_map>
#include <vector>
#include <iterator>
#include <utility>
#include <algorithm>
#include <cmath>

template <typename Edges, bool outside = true, bool anticlockwise = true>
class Connections {
	std::unordered_map<Edge, Edge, Edge::Hash> connections;
	using Connection = decltype(connections)::iterator;

	auto empty() const {
		return connections.empty();
	}

	auto follow(Connection connection) {
		return connections.find(connection->second);
	}

	auto unwind() {
		std::vector<Edge> edges;
		for (auto connection = connections.begin(); connection != connections.end(); connection = follow(connection)) {
			edges.push_back(connection->first);
			connections.erase(connection);
		}
		return edges;
	}

	struct Iterator {
		Connections &connections;
		Connection connection;

		Iterator(Connections &connections, const Connection &connection) : connections(connections), connection(connection) { }
		auto &operator++() { connection = connections.follow(connection); return *this;}
		auto &operator*() { return *connection; }
		auto operator->() { return connection; }
	};

public:
	Connections(const Edges &edges) {
		std::unordered_multimap<Point, Edge, Point::Hash> points_edges;
		std::transform(edges.begin(), edges.end(), std::inserter(points_edges, points_edges.begin()), [](const auto &edge) {
			return std::make_pair(edge.p0, edge);
		});

		auto ordering = [](const auto &pair1, const auto &pair2) {
			const auto &[edge1, angle1] = pair1;
			const auto &[edge2, angle2] = pair2;
			return angle1 < angle2;
		};

		for (const auto &incoming: edges) {
			std::vector<std::pair<Edge, double>> edges_angles;
			const auto &[start, stop] = points_edges.equal_range(incoming.p1);
			std::transform(start, stop, std::back_inserter(edges_angles), [&](const auto &point_edge) {
				const auto &[point, outgoing] = point_edge;
				const auto cross = incoming.delta() ^ outgoing.delta();
				const auto   dot = incoming.delta() * outgoing.delta();
				const auto angle = std::atan2(cross, dot);
				return std::make_pair(outgoing, angle);
			});
			const auto &[outgoing, angle] = outside
				? *std::max_element(edges_angles.begin(), edges_angles.end(), ordering)
				: *std::min_element(edges_angles.begin(), edges_angles.end(), ordering);
			connections.insert(anticlockwise ? std::make_pair(incoming, outgoing) : std::make_pair(-outgoing, -incoming));
		}
	}

	template <typename LessThan>
	auto begin(LessThan less_than) {
		const auto &connection = std::min_element(connections.begin(), connections.end(), [&](const auto &connection0, const auto &connection1) {
			const auto &[edge00, edge01] = connection0;
			const auto &[edge10, edge11] = connection1;
			return less_than(edge00.p1, edge10.p1);
		});
		return Iterator(*this, connection);
	}

	std::vector<Ring> rings() {
		std::vector<Ring> results;
		while (!empty())
			if (outside)
				for (auto &ring: Connections<decltype(unwind()), false, anticlockwise>(unwind()).rings())
					results.push_back(ring);
			else
				results.push_back(Ring(unwind()));
		return results;
	}
};

#endif
