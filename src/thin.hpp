#ifndef THIN_HPP
#define THIN_HPP

#include "point.hpp"
#include "tile.hpp"
#include <vector>
#include <utility>
#include <cstdint>
#include <algorithm>
#include <functional>

struct Thin {
	using Points = std::vector<Point>;

	double resolution;

	Thin(double resolution) : resolution(resolution) { }

	auto operator()(const Point &p1, const Point &p2) const {
		return
			std::pair<std::int32_t, std::int32_t>(p1[0] / resolution, p1[1] / resolution) <
			std::pair<std::int32_t, std::int32_t>(p2[0] / resolution, p2[1] / resolution);
	}

	auto operator()() { return Points(); }

	template <typename Classes>
	auto operator()(Tile &&tile, const Classes &classes) {
		auto points = Points();
		points.reserve(tile.size());
		for (const auto point: tile)
			if (!point.withheld && (point.key_point || classes.count(point.classification)))
				points.push_back(point);
		std::sort(points.begin(), points.end(), *this);

		auto here = points.begin(), points_end = points.end();
		for (auto range_begin = points.begin(); range_begin != points_end; ++here) {
			auto range_end = std::upper_bound(range_begin, points_end, *range_begin, *this);
			*here = *std::min_element(range_begin, range_end, std::greater());
			range_begin = range_end;
		}

		points.erase(here, points_end);
		return points;
	}

	auto operator()(const Points &points1, const Points &points2) {
		auto points = Points();
		points.reserve(points1.size() + points2.size());
		for (auto here1 = points1.begin(), here2 = points2.begin(), end1 = points1.end(), end2 = points2.end(); here1 != end1 || here2 != end2; ) {
			for (; here1 != end1 && (here2 == end2 || (*this)(*here1, *here2)); ++here1)
				points.push_back(*here1);
			for (; here2 != end2 && (here1 == end1 || (*this)(*here2, *here1)); ++here2)
				points.push_back(*here2);
			if (here1 != end1 && here2 != end2)
				points.push_back(*here1 > *here2 ? *here1 : *here2);
			if (here1 != end1) ++here1;
			if (here2 != end2) ++here2;
		}
		return points;
	}
};

#endif
