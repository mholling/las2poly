////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "triangle.hpp"
#include "triangles.hpp"
#include <unordered_set>
#include <algorithm>

using Edges = std::unordered_set<Edge>;

auto &operator-=(Edges &edges, Triangle const &triangle) {
	for (auto const &edge: triangle)
		if (!edges.erase(edge))
			edges.insert(-edge);
	return edges;
}

auto operator||(Edges const &edges, Triangles const &triangles) {
	return std::any_of(triangles.begin(), triangles.end(), [&](auto const &triangle) {
		return std::any_of(triangle.begin(), triangle.end(), [&](auto const &edge) {
			return edges.contains(edge);
		});
	});
}

#endif
