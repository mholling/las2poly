#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "triangle.hpp"
#include "triangles.hpp"
#include <unordered_set>
#include <algorithm>

using Edges = std::unordered_set<Edge>;

auto &operator-=(Edges &edges, const Triangle &triangle) {
	for (const auto &edge: triangle)
		if (!edges.erase(edge))
			edges.insert(-edge);
	return edges;
}

auto operator||(const Edges &edges, const Triangles &triangles) {
	return std::any_of(triangles.begin(), triangles.end(), [&](const auto &triangle) {
		return std::any_of(triangle.begin(), triangle.end(), [&](const auto &edge) {
			return edges.count(edge) > 0;
		});
	});
}

#endif
