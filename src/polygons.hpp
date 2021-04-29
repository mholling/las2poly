////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYGONS_HPP
#define POLYGONS_HPP

#include "rtree.hpp"
#include "polygon.hpp"
#include "triangles.hpp"
#include "summation.hpp"
#include "mesh.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include "ring.hpp"
#include <vector>
#include <utility>
#include <cmath>
#include <set>
#include <algorithm>
#include <iterator>
#include <ostream>

class Polygons : public std::vector<Polygon> {
	using Corner = Ring::CornerIterator;

	struct CompareAreas {
		const bool erode;
		CompareAreas(bool erode) : erode(erode) { }

		auto operator()(Corner const &corner) const {
			auto const cross = corner.cross();
			return std::pair(erode == (cross < 0), std::abs(cross));
		}

		auto operator()(Corner const &corner1, Corner const &corner2) const {
			return (*this)(corner1) < (*this)(corner2);
		}

		auto operator()(double corner_area) const {
			return std::pair(false, 2 * corner_area);
		}
	};

	struct CompareAngles {
		auto operator()(Corner const &corner) const {
			return corner.cosine();
		}

		auto operator()(Corner const &corner1, Corner const &corner2) const {
			return (*this)(corner1) < (*this)(corner2);
		}
	};

	void simplify_one_sided(double tolerance, bool erode) {
		auto const compare = CompareAreas(erode);
		auto const limit = compare(tolerance);
		auto queue = std::multiset<Corner, CompareAreas>(compare);
		for (auto &polygon: *this)
			for (auto &ring: polygon)
				for (auto corner = ring.begin(); corner != ring.end(); ++corner)
					queue.insert(corner);
		while (!queue.empty() && compare(*queue.begin()) < limit) {
			auto const least = queue.begin();
			auto const corner = *least;
			auto const prev = corner.prev();
			auto const next = corner.next();
			queue.erase(least);
			if (corner.ring_size() > 4) {
				queue.erase(prev);
				queue.erase(next);
				corner.remove();
				queue.insert(next.prev());
				queue.insert(prev.next());
			}
		}
	}

	void simplify(double tolerance, bool open) {
		simplify_one_sided(tolerance, !open);
		simplify_one_sided(tolerance, open);
	}

	void smooth(double tolerance, double angle) {
		auto queue = std::multiset<Corner, CompareAngles>();
		for (auto &polygon: *this)
			for (auto &ring: polygon)
				for (auto corner = ring.begin(); corner != ring.end(); ++corner)
					queue.insert(corner);
		for (auto const cosine = std::cos(angle); !queue.empty() && queue.begin()->cosine() < cosine; ) {
			auto const least = queue.begin();
			auto const corner = *least;
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const [v0, v1, v2] = *corner;
			auto const f0 = std::min(0.25, tolerance / (v1 - v0).norm());
			auto const f2 = std::min(0.25, tolerance / (v2 - v1).norm());
			auto const v10 = v0 * f0 + v1 * (1.0 - f0);
			auto const v11 = v2 * f2 + v1 * (1.0 - f2);
			queue.erase(least);
			corner.replace(v10, v11);
			queue.insert(next.prev());
			queue.insert(prev.next());
		}
	}

	auto static is_water(Triangles const &triangles, double delta, double slope) {
		auto perp_sum = Vector<3>{{0.0, 0.0, 0.0}};
		auto delta_sum = 0.0;
		auto count = 0ul;

		auto perp_sum_z = Summation(perp_sum[2]);
		auto abs_sum = Summation(delta_sum);

		for (auto edges: triangles) {
			std::rotate(edges.begin(), std::min_element(edges.begin(), edges.end(), [](auto const &edge1, auto const &edge2) {
				return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
			}), edges.end());
			auto const perp = edges[1] ^ edges[2];

			if (edges[0].first->withheld || edges[1].first->withheld || edges[2].first->withheld) {
				perp_sum_z += perp.norm();
				count += 2;
			} else {
				perp_sum[0] += perp[0];
				perp_sum[1] += perp[1];
				perp_sum_z  += perp[2];

				for (auto edge = edges.begin() + 1; edge != edges.end(); ++edge)
					if (edge->first->ground() && edge->second->ground())
						++count, abs_sum += std::abs(edge->second->elevation - edge->first->elevation);
			}
		}

		return count > 0 && delta_sum < delta * count && std::acos(std::abs(perp_sum[2] / perp_sum.norm())) < slope;
	}

public:
	Polygons(Mesh &mesh, double length, double width, double slope, double area, bool water, bool simplify, bool smooth, unsigned threads) {
		auto large_triangles = Triangles();
		auto outside_edges = Edges();
		auto const delta = width * std::tan(slope);

		mesh.deconstruct(large_triangles, outside_edges, length, threads);
		if (water)
			outside_edges.clear();

		large_triangles.explode([=, &outside_edges](auto const &&triangles) {
			if ((outside_edges || triangles) || ((width <= length || triangles > width) && is_water(triangles, delta, slope)))
				for (auto const &triangle: triangles)
					water ? outside_edges += triangle : outside_edges -= triangle;
		});

		auto rings = Rings(outside_edges);
		auto rings_end = std::remove_if(rings.begin(), rings.end(), [=](auto const &ring) {
			return ring < area && ring > -area;
		});
		auto holes_begin = std::partition(rings.begin(), rings_end, [](auto const &ring) {
			return ring > 0;
		});
		std::sort(rings.begin(), holes_begin);

		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&](auto const &exterior) {
			auto polygon = Polygon{{exterior}};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings_end, [&](auto const &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(polygon));
			emplace_back(polygon);
		});

		if (simplify) {
			auto const tolerance = 4 * width * width;
			this->simplify(tolerance, water);
		}

		if (smooth) {
			auto static constexpr pi = 3.14159265358979324;
			auto static constexpr angle = 15.0 * pi / 180;
			auto const tolerance = 0.5 * width / std::sin(angle);
			this->smooth(tolerance, angle);
		}
	}
};

auto &operator<<(std::ostream &json, Polygons const &polygons) {
	auto separator = '[';
	for (auto const &polygon: polygons)
		json << std::exchange(separator, ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
	return json << (separator == '[' ? "[]" : "]");
}

#endif
