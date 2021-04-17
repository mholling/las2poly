////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

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

	auto static unwind(Connections &connections) {
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
	Rings(Edges const &edges) {
		auto points_edges = PointsEdges();
		auto connections = Connections();

		for (auto const &edge: edges)
			points_edges.emplace(edge.first, edge);
		for (auto const &incoming: edges) {
			auto const ordering = [&](auto const &point_edge1, auto const &point_edge2) {
				auto const &p1 = point_edge1.second.second;
				auto const &p2 = point_edge2.second.second;
				return incoming < p1
					? incoming > p2 || Edge(p1, p2) > incoming.second
					: incoming > p2 && Edge(p1, p2) > incoming.second;
			};
			auto const &[start, stop] = points_edges.equal_range(incoming.second);
			auto const &[point, outgoing] = outside
				? *std::max_element(start, stop, ordering)
				: *std::min_element(start, stop, ordering);
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
