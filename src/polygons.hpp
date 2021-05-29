////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYGONS_HPP
#define POLYGONS_HPP

#include "ring.hpp"
#include "simplify.hpp"
#include "smooth.hpp"
#include "triangles.hpp"
#include "summation.hpp"
#include "mesh.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include "ring.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>

using Polygon = std::vector<Ring>;

class Polygons : public std::vector<Polygon>, public Simplify<Polygons>, public Smooth<Polygons> {
	auto static is_water(Triangles const &triangles, double slope) {
		auto perp_sum = Vector<3>{{0.0, 0.0, 0.0}};
		auto perp_sum_z = Summation(perp_sum[2]);

		for (auto edges: triangles) {
			std::rotate(edges.begin(), std::min_element(edges.begin(), edges.end(), [](auto const &edge1, auto const &edge2) {
				return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
			}), edges.end());

			auto const perp = edges[1] ^ edges[2];
			auto const &p0 = *edges[0].first;
			auto const &p1 = *edges[1].first;
			auto const &p2 = *edges[2].first;

			if (p0.withheld || p1.withheld || p2.withheld)
				perp_sum_z += perp.norm();
			else if (p0.is_ground() && p1.is_ground() && p2.is_ground()) {
				perp_sum[0] += perp[0];
				perp_sum[1] += perp[1];
				perp_sum_z  += perp[2];
			}
		}

		auto const perp_norm = perp_sum.norm();
		return perp_norm > 0 && std::acos(std::abs(perp_sum[2] / perp_norm)) < slope;
	}

	bool ogc;

public:
	Polygons(Mesh &mesh, double length, double width, double slope, bool water, bool ogc, int threads) : ogc(ogc) {
		auto large_triangles = Triangles();
		auto outside_edges = Edges();

		mesh.deconstruct(large_triangles, outside_edges, length, ogc != water, threads);
		if (water)
			outside_edges.clear();

		large_triangles.explode([=, &outside_edges](auto const &&triangles) {
			if ((outside_edges || triangles) || ((width <= length || triangles > width) && is_water(triangles, slope)))
				for (auto const &triangle: triangles)
					outside_edges -= triangle;
		});

		auto rings = Rings(outside_edges, ogc);
		auto holes_begin = std::partition(rings.begin(), rings.end(), [ogc](auto const &ring) {
			return ring.anticlockwise() == ogc;
		});
		std::sort(rings.begin(), holes_begin, [ogc](auto const &ring1, auto const &ring2) {
			return ring1.signed_area(ogc) < ring2.signed_area(ogc);
		});

		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&](auto const &exterior) {
			auto polygon = Polygon{{exterior}};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings.end(), [&](auto const &hole) {
				return exterior <=> hole != 0;
			});
			std::copy(old_remaining, remaining, std::back_inserter(polygon));
			emplace_back(polygon);
		});
	}

	void filter(double area) {
		erase(std::remove_if(begin(), end(), [area, this](auto &polygon) {
			polygon.erase(std::remove_if(std::next(polygon.begin()), polygon.end(), [area, this](auto const &ring) {
				return ring.signed_area(ogc) > -area;
			}), polygon.end());
			return polygon.front().signed_area(ogc) < area;
		}), end());
	}

	auto ring_count() const {
		return std::accumulate(begin(), end(), 0ul, [](auto const &sum, auto const &polygon) {
			return sum + polygon.size();
		});
	}
};

#endif
