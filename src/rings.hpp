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
	using Neighbours = std::unordered_multimap<PointIterator, PointIterator>;
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
		auto neighbours = Neighbours();
		auto connections = Connections();

		for (auto const &edge: edges)
			neighbours.emplace(edge.first, edge.second);
		for (auto const &incoming: edges) {
			auto points = std::vector<PointIterator>();
			auto const &[start, stop] = neighbours.equal_range(incoming.second);
			std::for_each(start, stop, [&](auto const &pair) {
				points.push_back(pair.second);
			});
			auto const ordering = [&](PointIterator const &p1, PointIterator const &p2) {
				return incoming < p1
					? incoming > p2 || Edge(p1, p2) > incoming.second
					: incoming > p2 && Edge(p1, p2) > incoming.second;
			};
			auto const &neighbour = outside
				? *std::max_element(points.begin(), points.end(), ordering)
				: *std::min_element(points.begin(), points.end(), ordering);
			auto const outgoing = Edge(incoming.second, neighbour);
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
