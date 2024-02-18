////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef EDGES_HPP
#define EDGES_HPP

#include "edge.hpp"
#include "triangles.hpp"
#include "summation.hpp"
#include "mesh.hpp"
#include "log.hpp"
#include "triangle.hpp"
#include <unordered_set>
#include <algorithm>
#include <cmath>

class Edges : public std::unordered_set<Edge> {
	auto static is_water(Triangles const &triangles, double delta, double slope) {
		auto perp_sum = Vector<3>{{0.0, 0.0, 0.0}};
		auto perp_sum_z = Summation(perp_sum[2]);

		auto delta_sum = 0.0;
		auto delta_count = 0ul;
		auto delta_summer = Summation(delta_sum);

		for (auto edges: triangles) {
			std::rotate(edges.begin(), std::min_element(edges.begin(), edges.end(), [](auto const &edge1, auto const &edge2) {
				return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
			}), edges.end());

			auto const perp = edges[1] ^ edges[2];
			auto const &p0 = *edges[0].first;
			auto const &p1 = *edges[1].first;
			auto const &p2 = *edges[2].first;

			if (p0.withheld || p1.withheld || p2.withheld) {
				perp_sum_z += perp.norm();
				delta_count += 2;
			} else if (p0.ground() && p1.ground() && p2.ground()) {
				perp_sum[0] += perp[0];
				perp_sum[1] += perp[1];
				perp_sum_z  += perp[2];
				delta_summer += std::abs(p1.elevation - p2.elevation);
				delta_summer += std::abs(p2.elevation - p0.elevation);
				delta_count += 2;
			}
		}

		return delta_sum < delta * delta_count && std::abs(perp_sum[2]) > std::cos(slope) * perp_sum.norm();
	}

public:
	Edges(Mesh &mesh, double width, double delta, double slope, bool water, bool ogc, int threads, Log &log) {
		auto large_triangles = Triangles();

		log(Log::Time(), "extracting polygon edges");
		mesh.deconstruct(large_triangles, *this, width, ogc != water, threads);

		if (water)
			clear();

		large_triangles.explode([=, this](auto const &&triangles) {
			if ((*this || triangles) || is_water(triangles, delta, slope))
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
