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
	auto &operator-=(Triangle const &triangle) {
		for (auto const &edge: triangle)
			if (!erase(edge))
				insert(-edge);
		return *this;
	}

	auto operator&(Triangles const &triangles) const {
		return std::any_of(triangles.begin(), triangles.end(), [&](auto const &triangle) {
			return std::any_of(triangle.begin(), triangle.end(), [&](auto const &edge) {
				return contains(edge);
			});
		});
	}

	Edges(App const &app, Mesh &mesh) {
		auto large_triangles = Triangles();

		app.log("extracting boundaries");
		mesh.deconstruct(app, large_triangles, *this);

		if (!app.land)
			clear();

		for (auto triangles: large_triangles.grouped())
			if (*this & triangles || triangles.is_water(app))
				for (auto const &triangle: triangles)
					*this -= triangle;
	}
};

#endif
