////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POINTS_HPP
#define POINTS_HPP

#include "point.hpp"
#include "bounds.hpp"
#include "srs.hpp"
#include "tile.hpp"
#include "app.hpp"
#include "fill.hpp"
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <set>
#include <limits>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <functional>
#include <iterator>
#include <mutex>
#include <iostream>
#include <fstream>
#include <optional>
#include <thread>
#include <numeric>
#include <cstddef>

class Points : public std::vector<Point> {
	using Paths = std::vector<std::filesystem::path>;
	using PathIterator = Paths::const_iterator;
	using Discard = std::unordered_set<unsigned char>;

	std::vector<Bounds> tile_bounds;
	std::set<OptionalSRS> distinct_srs;

	void update(Tile &tile) {
		if (tile.bounds.empty())
			return;
		tile_bounds.push_back(tile.bounds);
		distinct_srs.insert(tile.srs());
	}

	void update(Points &points) {
		tile_bounds.insert(tile_bounds.end(), points.tile_bounds.begin(), points.tile_bounds.end());
		distinct_srs.insert(points.distinct_srs.begin(), points.distinct_srs.end());
	}

	struct Thin {
		auto static constexpr web_mercator_range = 40097932.2;
		auto static constexpr min_resolution = web_mercator_range / std::numeric_limits<int>::max();

		double resolution;

		Thin(double resolution) : resolution(resolution) {
			if (resolution < min_resolution)
				throw std::runtime_error("width value too small");
		}

		auto operator()(Point const &p1, Point const &p2) const {
			return
				std::pair<int, int>(p1[0] / resolution, p1[1] / resolution) <
				std::pair<int, int>(p2[0] / resolution, p2[1] / resolution);
		}

		void operator()(App const &app, Points &points, Tile &&tile) const {
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

public:
	Points(App const &app, PathIterator begin, PathIterator end, Thin const &thin, std::mutex &mutex, std::exception_ptr &exception, int threads) {
		if (auto lock = std::lock_guard(mutex); exception)
			return;
		try {
			auto const middle = begin + (end - begin) / 2;
			if (begin + 1 == end) {
				auto const &path = *begin;
				try {
					if (path == "-") {
						std::cin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						thin(app, *this, Tile(std::cin));
					} else {
						auto input = std::ifstream(path, std::ios::binary);
						input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						thin(app, *this, Tile(input));
					}
				} catch (std::ios_base::failure &) {
					throw std::runtime_error(path.string() + ": problem reading file");
				} catch (std::runtime_error &error) {
					throw std::runtime_error(path.string() + ": " + error.what());
				}
			} else if (1 == threads) {
				auto points1 = Points(app, begin, middle, thin, mutex, exception, 1);
				auto points2 = Points(app, middle, end, thin, mutex, exception, 1);
				thin(*this, points1, points2);
			} else {
				auto points1 = std::optional<Points>();
				auto points2 = std::optional<Points>();
				auto thread1 = std::thread([&]() {
					points1.emplace(app, begin, middle, thin, mutex, exception, threads/2);
				}), thread2 = std::thread([&]() {
					points2.emplace(app, middle, end, thin, mutex, exception, threads - threads/2);
				});
				thread1.join(), thread2.join();
				thin(*this, *points1, *points2);
			}
			if (app.srs)
				distinct_srs = {app.srs};
			if (distinct_srs.size() > 1)
				throw std::runtime_error("dissimilar SRS or EPSG codes detected");
		} catch (std::runtime_error &) {
			auto lock = std::lock_guard(mutex);
			exception = std::current_exception();
		}
	}

	Points(App const &app) {
		auto const thin = Thin(app.resolution);
		auto mutex = std::mutex();
		auto exception = std::exception_ptr();

		app.log("reading", app.tile_paths.size(), "file");
		*this = Points(app, app.tile_paths.begin(), app.tile_paths.end(), thin, mutex, exception, app.threads);

		if (exception)
			std::rethrow_exception(exception);

		if (!app.land && size() > 2) {
			app.log("synthesising surrounding points");
			auto const overall_bounds = std::accumulate(tile_bounds.begin(), tile_bounds.end(), Bounds());
			auto fill = Fill(overall_bounds, app.resolution);

			for (auto const &bounds: tile_bounds)
				fill(bounds);

			fill([this](auto x, auto y) {
				emplace_back(x, y, 0.0, 2, false, true, false);
			});
		}
	}

	auto srs() const {
		return distinct_srs.empty() ? OptionalSRS() : *distinct_srs.begin();
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(PointIterator const &point) const { return std::hash<Point *>()(&*point); }
};

#endif
