#ifndef RINGS_HPP
#define RINGS_HPP

#include "points.hpp"
#include "edge.hpp"
#include "ring.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

template <bool outside = true>
class Rings : public std::vector<Ring> {
	using PointsEdges = std::unordered_multimap<PointIterator, Edge>;
	using Connections = std::unordered_map<Edge, Edge>;

	static auto unwind(Connections &connections) {
		std::vector<Edge> edges;
		for (auto connection = connections.begin(); connection != connections.end(); ) {
			edges.push_back(connection->first);
			connections.erase(connection);
			connection = connections.find(connection->second);
		}
		return edges;
	}

public:
	template <typename Edges>
	Rings(const Edges &edges) {
		PointsEdges points_edges;
		Connections connections;

		for (const auto &edge: edges)
			points_edges.emplace(edge.first, edge);
		auto ordering = [](const auto &pair1, const auto &pair2) {
			const auto &[edge1, angle1] = pair1;
			const auto &[edge2, angle2] = pair2;
			return angle1 < angle2;
		};
		for (const auto &incoming: edges) {
			std::unordered_map<Edge, double> edges_angles;
			const auto &[start, stop] = points_edges.equal_range(incoming.second);
			std::for_each(start, stop, [&](const auto &point_edge) {
				const auto &[point, outgoing] = point_edge;
				const auto cross = incoming ^ outgoing;
				const auto   dot = incoming * outgoing;
				const auto angle = std::atan2(cross, dot);
				edges_angles.emplace(outgoing, angle);
			});
			const auto &[outgoing, angle] = outside
				? *std::max_element(edges_angles.begin(), edges_angles.end(), ordering)
				: *std::min_element(edges_angles.begin(), edges_angles.end(), ordering);
			connections.emplace(incoming, outgoing);
		}

		while (!connections.empty())
			if constexpr (outside)
				for (auto &ring: Rings<!outside>(unwind(connections)))
					push_back(ring);
			else
				emplace_back(unwind(connections));
	}
};

#endif
