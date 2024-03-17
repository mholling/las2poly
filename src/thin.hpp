////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef THIN_HPP
#define THIN_HPP

#include "point.hpp"
#include "app.hpp"
#include "tile.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
#include <istream>
#include <algorithm>
#include <functional>
#include <iterator>

struct Thin {
	auto static constexpr web_mercator_range = 40097932.2;
	auto static constexpr min_resolution = web_mercator_range / std::numeric_limits<int>::max();

	double resolution;

	Thin(double resolution = min_resolution) : resolution(resolution) {
		if (resolution < min_resolution)
			throw std::runtime_error("width value too small");
	}

	auto operator()(Point const &p1, Point const &p2) const {
		return
			std::pair<int, int>(p1[0] / resolution, p1[1] / resolution) <
			std::pair<int, int>(p2[0] / resolution, p2[1] / resolution);
	}

	template <typename Points>
	void operator()(App const &app, Points &points, std::istream &input) const {
		auto tile = Tile(input);
		points.reserve(tile.size());

		for (auto const point: tile)
			if (!point.withheld && (point.key_point || !app.discard.contains(point.classification)))
				points.push_back(point);
		std::sort(points.begin(), points.end(), *this);

		auto here = points.begin();
		auto const points_end = points.end();
		for (auto range_begin = points.begin(); range_begin != points_end; ++here) {
			auto const range_end = std::upper_bound(range_begin, points_end, *range_begin, *this);
			*here = *std::min_element(range_begin, range_end, std::greater());
			range_begin = range_end;
		}

		points.erase(here, points_end);
		points.update(tile);
	}

	template <typename Points>
	void operator()(Points &points, Points &points1, Points &points2) const {
		points.reserve(points1.size() + points2.size());

		auto point1 = points1.begin(), point2 = points2.begin();
		auto const end1 = points1.end(), end2 = points2.end();
		while (point1 != end1 && point2 != end2)
			if ((*this)(*point1, *point2))
				points.push_back(*point1++);
			else if ((*this)(*point2, *point1))
				points.push_back(*point2++);
			else {
				points.push_back(*point1 > *point2 ? *point1 : *point2);
				++point1, ++point2;
			}
		std::copy(point1, end1, std::back_inserter(points));
		std::copy(point2, end2, std::back_inserter(points));

		points.update(points1);
		points.update(points2);
	}
};

#endif
