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
#include "app.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <cmath>

using Polygon = std::vector<Ring>;
using MultiPolygon = std::vector<Polygon>;

class Polygons : public MultiPolygon, public Simplify<Polygons>, public Smooth<Polygons> {
	Polygons(Rings &&rings) {
		auto holes_begin = std::partition(rings.begin(), rings.end(), [](auto const &ring) {
			return ring.exterior();
		});
		std::sort(rings.begin(), holes_begin, [](auto const &ring1, auto const &ring2) {
			return ring1.signed_area() < ring2.signed_area();
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

public:
	Polygons() = default;

	auto ring_count() const {
		return std::accumulate(begin(), end(), 0ul, [](auto const &sum, auto const &polygon) {
			return sum + polygon.size();
		});
	}

	auto multipolygon() const {
		return static_cast<MultiPolygon const &>(*this);
	}

	auto linestrings() const {
		auto collection = Linestrings();
		for (auto const &polygon: *this)
			for (auto const &ring: polygon)
				collection.push_back(ring);
		return collection;
	}

	auto multilinestrings() const {
		auto collection = MultiLinestrings();
		for (auto const &polygon: *this)
			for (auto &multi = collection.emplace_back(); auto const &ring: polygon)
				multi.push_back(ring);
		return collection;
	}

	template <typename Edges>
	Polygons(Edges const &edges, bool allow_self_intersection) :
		Polygons(Rings(edges, allow_self_intersection))
	{ }

	Polygons(App const &app, Edges const &edges) :
		Polygons(edges, !app.land)
	{
		if (app.simplify) {
			app.log("simplifying", ring_count(), "ring");
			auto const tolerance = 4 * *app.width * *app.width;
			simplify(tolerance, app.land);
		}

		if (app.smooth) {
			app.log("smoothing", ring_count(), "ring");
			smooth(app.land);
		}

		if (auto const min_area = *app.area; min_area > 0)
			erase(std::remove_if(begin(), end(), [&](auto &polygon) {
				polygon.erase(std::remove_if(std::next(polygon.begin()), polygon.end(), [&](auto const &ring) {
					return ring.signed_area() > -min_area;
				}), polygon.end());
				return polygon.front().signed_area() < min_area;
			}), end());
	}

	auto reassemble(bool allow_self_intersection) const {
		auto segments = Segments();
		for (auto const &polygon: *this)
			for (auto const &ring: polygon)
				for (auto const &[v0, v1, v2]: ring.corners())
					segments.emplace_back(v1, v2);
		return Polygons(segments, allow_self_intersection);
	}
};

#endif
