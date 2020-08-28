#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP

#include "point.hpp"
#include "edge.hpp"
#include "ring.hpp"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <utility>
#include <cmath>

class Connections {
	std::unordered_map<Edge, Edge, Edge::Hash> connections;
	bool outside;

	auto empty() const {
		return connections.empty();
	}

	auto unwind() {
		std::vector<Edge> edges;
		for (auto connection = connections.begin(); connection != connections.end();) {
			const auto [incoming, outgoing] = *connection;
			edges.push_back(incoming);
			connections.erase(connection);
			connection = connections.find(outgoing);
		}
		return edges;
	}

	template <typename C>
	Connections(const C &edges, bool outside) : outside(outside) {
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
			const auto &[outgoing, angle] = outside ?
				*std::max_element(edges_angles.begin(), edges_angles.end(), ordering) :
				*std::min_element(edges_angles.begin(), edges_angles.end(), ordering);
			connections.insert(std::make_pair(incoming, outgoing));
		}
	}

public:
	template <typename C>
	Connections(const C &edges) : Connections(edges, true) { }

	auto rings() {
		std::vector<Ring> results;
		while (!empty()) {
			Connections inside(unwind(), !outside);
			while (!inside.empty())
				results.push_back(Ring(inside.unwind()));
		}
		return results;
	}
};

#endif
