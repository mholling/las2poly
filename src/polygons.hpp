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
#include "edges.hpp"
#include "rings.hpp"
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>

using Polygon = std::vector<Ring>;
using MultiPolygon = std::vector<Polygon>;

struct Polygons : public MultiPolygon, public Simplify<Polygons>, public Smooth<Polygons> {
	Polygons() = default;

	Polygons(Edges const &edges, bool ogc) {
		auto rings = Rings(edges, ogc);
		auto holes_begin = std::partition(rings.begin(), rings.end(), [ogc](auto const &ring) {
			return ring.anticlockwise() == ogc;
		});
		std::sort(rings.begin(), holes_begin, [ogc](auto const &ring1, auto const &ring2) {
			return ring1.signed_area(ogc) < ring2.signed_area(ogc);
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
	}

	void filter(double area, bool ogc) {
		erase(std::remove_if(begin(), end(), [=](auto &polygon) {
			polygon.erase(std::remove_if(std::next(polygon.begin()), polygon.end(), [=](auto const &ring) {
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

	auto &multi() const {
		return static_cast<MultiPolygon const &>(*this);
	}
};

#endif
