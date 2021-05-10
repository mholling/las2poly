////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POINTS_HPP
#define POINTS_HPP

#include "point.hpp"
#include "bounds.hpp"
#include "tile.hpp"
#include "fill.hpp"
#include <vector>
#include <string>
#include <unordered_set>
#include <limits>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <functional>
#include <mutex>
#include <iostream>
#include <fstream>
#include <thread>
#include <numeric>
#include <cstddef>

class Points : public std::vector<Point> {
	using Paths = std::vector<std::string>;
	using PathIterator = Paths::const_iterator;
	using Discard = std::unordered_set<unsigned char>;

	std::vector<Bounds> tile_bounds;

	Points() = default;

	void update(Tile const &tile) {
		if (!tile.bounds.empty())
			tile_bounds.push_back(tile.bounds);
	}

	void update(Points &points) {
		tile_bounds.insert(tile_bounds.end(), points.tile_bounds.begin(), points.tile_bounds.end());
	}

	void swap(Points &other) {
		vector::swap(other);
		std::swap(tile_bounds, other.tile_bounds);
	}

	struct Thin {
		double resolution;

		Thin(double resolution) : resolution(resolution) {
			auto static constexpr web_mercator_range = 2 * 20048966.10;
			if (web_mercator_range / resolution > std::numeric_limits<int>::max())
				throw std::runtime_error("resolution value too small");
		}

		auto operator()(Point const &p1, Point const &p2) const {
			return
				std::pair<int, int>(p1[0] / resolution, p1[1] / resolution) <
				std::pair<int, int>(p2[0] / resolution, p2[1] / resolution);
		}

		void operator()(Points &points, Tile &&tile, Discard const &discard) const {
			points.reserve(tile.size());

			for (auto const point: tile)
				if (!point.withheld && (point.key_point || !discard.contains(point.classification)))
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

			points.update(points1);
			points.update(points2);
		}
	};

	Points(PathIterator begin, PathIterator end, double resolution, Discard const &discard, std::mutex &mutex, std::exception_ptr &exception, unsigned threads) {
		if (auto lock = std::lock_guard(mutex); exception)
			return;
		auto thin = Thin(resolution);
		auto const middle = begin + (end - begin) / 2;
		if (begin + 1 == end)
			try {
				auto const &path = *begin;
				try {
					if (path == "-") {
						std::cin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						thin(*this, Tile(std::cin), discard);
					} else {
						auto input = std::ifstream(path, std::ios::binary);
						input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						thin(*this, Tile(input), discard);
					}
				} catch (std::ios_base::failure &) {
					throw std::runtime_error(path + ": problem reading file");
				} catch (std::runtime_error &error) {
					throw std::runtime_error(path + ": " + error.what());
				}
			} catch (std::runtime_error &) {
				auto lock = std::lock_guard(mutex);
				exception = std::current_exception();
				return;
			}
		else if (1 == threads) {
			auto points1 = Points(begin, middle, resolution, discard, mutex, exception, 1);
			auto points2 = Points(middle, end, resolution, discard, mutex, exception, 1);
			thin(*this, points1, points2);
		} else {
			auto points1 = Points();
			auto points2 = Points();
			auto thread1 = std::thread([&]() {
				Points(begin, middle, resolution, discard, mutex, exception, threads/2).swap(points1);
			}), thread2 = std::thread([&]() {
				Points(middle, end, resolution, discard, mutex, exception, threads - threads/2).swap(points2);
			});
			thread1.join(), thread2.join();
			thin(*this, points1, points2);
		}
	}

public:
	Points(Paths const &tile_paths, double resolution, std::vector<int> const &discard_ints, bool water, unsigned threads) {
		auto discard = Discard(discard_ints.begin(), discard_ints.end());
		auto mutex = std::mutex();
		auto exception = std::exception_ptr();

		Points(tile_paths.begin(), tile_paths.end(), resolution, discard, mutex, exception, threads).swap(*this);

		if (exception)
			std::rethrow_exception(exception);

		if (water && size() > 2) {
			auto const overall_bounds = std::accumulate(tile_bounds.begin(), tile_bounds.end(), Bounds());
			auto fill = Fill(overall_bounds, resolution);

			for (auto const &bounds: tile_bounds)
				fill(bounds);

			fill([&](auto x, auto y) {
				emplace_back(x, y, 0.0, 2, false, true, false);
			});
		}
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(PointIterator const &point) const { return std::hash<Point *>()(&*point); }
};

#endif
