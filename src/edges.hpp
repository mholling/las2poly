////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "app.hpp"
#include "triangles.hpp"
#include "mesh.hpp"
#include "triangle.hpp"
#include <unordered_set>
#include <algorithm>

struct Edges : std::unordered_set<Edge> {
	Edges(App const &app, Mesh &mesh) {
		auto large_triangles = Triangles();

		app.log("extracting polygon edges");
		mesh.deconstruct(app, large_triangles, *this);

		if (!app.land)
			clear();

		large_triangles.explode([=, this](auto const &&triangles) {
			if ((*this || triangles) || app.is_water(triangles))
				for (auto const &triangle: triangles)
					*this -= triangle;
		});
	}
};

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
