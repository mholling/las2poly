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

struct Polygons : public MultiPolygon, public Simplify<Polygons>, public Smooth<Polygons> {
	Polygons() = default;

	auto ring_count() const {
		return std::accumulate(begin(), end(), 0ul, [](auto const &sum, auto const &polygon) {
			return sum + polygon.size();
		});
	}

	auto &multi() const {
		return static_cast<MultiPolygon const &>(*this);
	}

	Polygons(App const &app, Edges const &edges) {
		auto rings = Rings(edges, app.ogc);
		auto holes_begin = std::partition(rings.begin(), rings.end(), [&app](auto const &ring) {
			return ring.anticlockwise() == app.ogc;
		});
		std::sort(rings.begin(), holes_begin, [&app](auto const &ring1, auto const &ring2) {
			return ring1.signed_area(app.ogc) < ring2.signed_area(app.ogc);
		});

		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&, this](auto const &exterior) {
			auto polygon = Polygon{{exterior}};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings.end(), [&](auto const &hole) {
				return exterior <=> hole != 0;
			});
			std::copy(old_remaining, remaining, std::back_inserter(polygon));
			emplace_back(polygon);
		});

		if (app.simplify || app.smooth) {
			app.log(app.smooth ? "smoothing" : "simplifying", ring_count(), "ring");
			auto const tolerance = 4 * app.width * app.width;
			simplify(tolerance, app.land ? !app.ogc : app.ogc, app.threads);
		}

		if (app.smooth) {
			auto const tolerance = 0.5 * app.width / std::sin(app.angle);
			smooth(tolerance, app.angle, app.threads);
		}

		if (app.area > 0)
			erase(std::remove_if(begin(), end(), [=](auto &polygon) {
				polygon.erase(std::remove_if(std::next(polygon.begin()), polygon.end(), [&app](auto const &ring) {
					return ring.signed_area(app.ogc) > -app.area;
				}), polygon.end());
				return polygon.front().signed_area(app.ogc) < app.area;
			}), end());
	}
};

#endif
