#ifndef EDGE_HPP
#define EDGE_HPP

#include "point.hpp"
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <vector>
#include <utility>
#include <cmath>

struct Edge {
	const Point &p0, &p1;

	Edge(const Point &p0, const Point &p1) : p0(p0), p1(p1) { }

	friend auto operator==(const Edge &edge1, const Edge &edge2) {
		return edge1.p0 == edge2.p0 && edge1.p1 == edge2.p1;
	}

	friend auto operator||(const Edge &edge1, const Edge &edge2) {
		return edge1.p0 == edge2.p1 && edge1.p1 == edge2.p0;
	}

	struct Hash {
		auto operator()(const Edge &edge) const { return Point::Hash()(edge.p0) ^ Point::Hash()(edge.p1); }
	};

	struct Opposite {
		bool operator()(const Edge &edge1, const Edge &edge2) const { return edge1 || edge2; }
	};

	auto opposite() const {
		return Edge(p1, p0);
	}

	auto operator&(const Point &p) const {
		return p0 == p || p1 == p;
	}

	auto operator<<(const Point &p) const {
		return (p0 < p) && !(p1 < p) && ((p0 - p) ^ (p1 - p)) > 0;
	}

	auto operator>>(const Point &p) const {
		return (p1 < p) && !(p0 < p) && ((p1 - p) ^ (p0 - p)) > 0;
	}

	auto operator^(const Point &p) const {
		return (p0 - p) ^ (p1 - p);
	}

	auto operator>(double length) const {
		return (p0 >> p1) > length;
	}

	auto delta() const {
		return p1 - p0;
	}

	template <bool outside, typename C>
	static auto connections_for(const C &edges) {
		std::unordered_multimap<Point, Edge, Point::Hash> points_edges;
		std::transform(edges.begin(), edges.end(), std::inserter(points_edges, points_edges.begin()), [](const auto &edge) {
			return std::make_pair(edge.p0, edge);
		});

		auto ordering = [](const auto &pair1, const auto &pair2) {
			const auto &[edge1, angle1] = pair1;
			const auto &[edge2, angle2] = pair2;
			return angle1 < angle2;
		};

		std::unordered_map<Edge, Edge, Hash> connections;
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
		return connections;
	}

	template <typename F>
	static void follow(std::unordered_map<Edge, Edge, Hash> &connections, F function) {
		for (auto connection = connections.begin(); connection != connections.end();) {
			const auto [incoming, outgoing] = *connection;
			function(incoming);
			connections.erase(connection);
			connection = connections.find(outgoing);
		}
	}
};

#endif
