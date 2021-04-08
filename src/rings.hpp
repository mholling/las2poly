#ifndef RINGS_HPP
#define RINGS_HPP

#include "ring.hpp"
#include "points.hpp"
#include "edge.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>

template <bool outside = true>
class Rings : public std::vector<Ring> {
	using PointsEdges = std::unordered_multimap<PointIterator, Edge>;
	using Connections = std::unordered_map<Edge, Edge>;

	static auto unwind(Connections &connections) {
		auto edges = std::vector<Edge>();
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
		auto points_edges = PointsEdges();
		auto connections = Connections();

		for (const auto &edge: edges)
			points_edges.emplace(edge.first, edge);
		for (const auto &incoming: edges) {
			auto edges = std::vector<Edge>();
			const auto &[start, stop] = points_edges.equal_range(incoming.second);
			std::for_each(start, stop, [&](const auto &point_edge) {
				edges.push_back(point_edge.second);
			});
			const auto ordering = [&](const Edge &edge1, const Edge &edge2) {
				return (incoming ^ edge1) < 0
					? (incoming ^ edge2) > 0 || (edge1 ^ edge2) > 0
					: (incoming ^ edge2) > 0 && (edge1 ^ edge2) > 0;
			};
			const auto &outgoing = outside
				? *std::max_element(edges.begin(), edges.end(), ordering)
				: *std::min_element(edges.begin(), edges.end(), ordering);
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
