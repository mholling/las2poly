#ifndef RINGS_HPP
#define RINGS_HPP

#include "point.hpp"
#include "edge.hpp"
#include "ring.hpp"
#include <unordered_map>
#include <vector>
#include <iterator>
#include <utility>
#include <algorithm>
#include <cmath>

template <typename Edges, bool outside = true>
class Rings {
	std::unordered_map<Edge, Edge> connections;

	auto empty() const {
		return connections.empty();
	}

	auto unwind() {
		std::vector<Edge> edges;
		for (auto connection = connections.begin(); connection != connections.end(); ) {
			edges.push_back(connection->first);
			connections.erase(connection);
			connection = connections.find(connection->second);
		}
		return edges;
	}

public:
	Rings(const Edges &edges) {
		std::unordered_multimap<const Point &, Edge> points_edges;
		std::transform(edges.begin(), edges.end(), std::inserter(points_edges, points_edges.begin()), [](const auto &edge) {
			return std::pair(edge.first, edge);
		});

		auto ordering = [](const auto &pair1, const auto &pair2) {
			const auto &[edge1, angle1] = pair1;
			const auto &[edge2, angle2] = pair2;
			return angle1 < angle2;
		};

		for (const auto &incoming: edges) {
			std::vector<std::pair<Edge, double>> edges_angles;
			const auto &[start, stop] = points_edges.equal_range(incoming.first);
			std::transform(start, stop, std::back_inserter(edges_angles), [&](const auto &point_edge) {
				const auto &[point, outgoing] = point_edge;
				const auto cross = incoming ^ outgoing;
				const auto   dot = incoming * outgoing;
				const auto angle = std::atan2(cross, dot);
				return std::pair(outgoing, angle);
			});
			const auto &[outgoing, angle] = outside
				? *std::max_element(edges_angles.begin(), edges_angles.end(), ordering)
				: *std::min_element(edges_angles.begin(), edges_angles.end(), ordering);
			connections.insert(std::pair(incoming, outgoing));
		}
	}

	auto operator()() {
		std::vector<Ring> results;
		while (!empty())
			if constexpr (outside)
				for (auto &ring: Rings<decltype(unwind()), false>(unwind())())
					results.push_back(ring);
			else
				results.push_back(Ring(unwind()));
		return results;
	}
};

#endif
