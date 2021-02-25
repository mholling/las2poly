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

template <typename Edges, bool outside = true>
class Connections {
	std::unordered_map<Edge, Edge, Edge::Hash> connections;

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
			connections.insert(std::make_pair(incoming, outgoing));
		}
	}

	std::vector<Ring> rings() {
		std::vector<Ring> results;
		while (!empty())
			if (outside)
				for (auto &ring: Connections<std::vector<Edge>, false>(unwind()).rings())
					results.push_back(ring);
			else
				results.push_back(Ring(unwind()));
		return results;
	}
};

#endif
