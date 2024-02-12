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
#include "edges.hpp"
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <algorithm>

struct Rings : std::vector<Ring> {
	using PointsEdges = std::unordered_multimap<PointIterator, Edge>;
	using Connections = std::unordered_map<Edge, Edge>;

	template <typename Edges, bool exterior = std::is_same_v<Edges, ::Edges>>
	Rings(Edges const &edges, bool ogc) {
		auto points_edges = PointsEdges();
		for (auto const &edge: edges)
			points_edges.emplace(edge.first, edge);

		auto connections = Connections();
		for (auto const &incoming: edges) {
			auto const ordering = [&](auto const &point_edge1, auto const &point_edge2) {
				auto const &p1 = point_edge1.second.second;
				auto const &p2 = point_edge2.second.second;
				return incoming < p1
					? incoming > p2 || Edge(p1, p2) > incoming.second
					: incoming > p2 && Edge(p1, p2) > incoming.second;
			};
			auto const [start, stop] = points_edges.equal_range(incoming.second);
			auto const &[point, outgoing] = exterior
				? *std::max_element(start, stop, ordering)
				: *std::min_element(start, stop, ordering);
			connections.emplace(incoming, outgoing);
		}

		auto interior_edges = std::vector<Edge>();
		while (!connections.empty()) {
			auto edges = std::vector<Edge>();
			for (auto connection = connections.begin(); connection != connections.end(); ) {
				edges.push_back(connection->first);
				connections.erase(std::exchange(connection, connections.find(connection->second)));
			}

			auto const ring = Ring(edges);
			if (!exterior || ring.anticlockwise() == ogc)
				push_back(ring);
			else
				interior_edges.insert(interior_edges.end(), edges.begin(), edges.end());
		}

		if constexpr (exterior) {
			auto const holes = Rings(interior_edges, ogc);
			insert(end(), holes.begin(), holes.end());
		}
	}
};

#endif
