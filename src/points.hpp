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
#include <set>
#include <cstddef>

class Points : public std::vector<Point> {
	using Paths = std::vector<std::string>;
	using Discard = std::unordered_set<unsigned char>;

	std::vector<Bounds> tile_bounds;

	Points() = default;

	void update(Tile const &tile) {
		tile_bounds.push_back(tile.bounds);
	}

	void update(Points &points) {
		tile_bounds.insert(tile_bounds.end(), points.tile_bounds.begin(), points.tile_bounds.end());
	}

	struct Thin {
		double resolution;

		Thin(double resolution) : resolution(resolution) {
			auto static constexpr web_mercator_max = 20048966.10;
			if (web_mercator_max / resolution > std::numeric_limits<int>::max())
				throw std::runtime_error("resolution value too small");
		}

		auto operator()(Point const &p1, Point const &p2) const {
			return
				std::pair<int, int>(p1[0] / resolution, p1[1] / resolution) <
				std::pair<int, int>(p2[0] / resolution, p2[1] / resolution);
		}

		auto operator()(Tile &&tile, Discard const &discard) const {
			auto points = Points();
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
			return points;
		}

		auto operator()(Points &points1, Points &points2) const {
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

			points.update(points1);
			points.update(points2);
			return points;
		}
	};

	class Load {
		using PathIterator = Paths::const_iterator;

		Thin thin;
		Discard discard;
		std::mutex mutex;
		std::exception_ptr exception;

		auto operator()(PathIterator begin, PathIterator end, unsigned threads) {
			if (auto lock = std::lock_guard(mutex); exception)
				return Points();
			auto const middle = begin + (end - begin) / 2;
			if (begin + 1 == end)
				try {
					auto const &path = *begin;
					try {
						if (path == "-") {
							std::cin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
							return thin(Tile(std::cin), discard);
						} else {
							auto input = std::ifstream(path, std::ios::binary);
							input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
							return thin(Tile(input), discard);
						}
					} catch (std::ios_base::failure &) {
						throw std::runtime_error(path + ": problem reading file");
					} catch (std::runtime_error &error) {
						throw std::runtime_error(path + ": " + error.what());
					}
				} catch (std::runtime_error &) {
					auto lock = std::lock_guard(mutex);
					exception = std::current_exception();
					return Points();
				}
			else if (1 == threads) {
				auto points1 = (*this)(begin, middle, 1);
				auto points2 = (*this)(middle, end, 1);
				return thin(points1, points2);
			} else {
				auto points1 = Points();
				auto points2 = Points();
				auto thread1 = std::thread([&]() {
					points1 = (*this)(begin, middle, threads/2);
				}), thread2 = std::thread([&]() {
					points2 = (*this)(middle, end, threads - threads/2);
				});
				thread1.join(), thread2.join();
				return thin(points1, points2);
			}
		}

	public:
		auto operator()(Paths const &paths, unsigned threads) {
			auto points = (*this)(paths.begin(), paths.end(), threads);
			if (exception)
				std::rethrow_exception(exception);
			return points;
		}

		template <typename Discard>
		Load(double resolution, Discard const &discard) : thin(resolution), discard(discard.begin(), discard.end()) { }
	};

	auto swap(Points &other) {
		vector::swap(other);
		std::swap(tile_bounds, other.tile_bounds);
	}

public:
	template <typename Discard>
	Points(Paths const &tile_paths, double resolution, Discard const &discard, bool water, unsigned threads) {
		Load(resolution, discard)(tile_paths, threads).swap(*this);

		if (water) {
			int const imin = std::min_element(tile_bounds.begin(), tile_bounds.end(), Bounds::CompareYMin())->ymin / resolution;
			int const jmin = std::min_element(tile_bounds.begin(), tile_bounds.end(), Bounds::CompareXMin())->xmin / resolution;
			int const imax = std::max_element(tile_bounds.begin(), tile_bounds.end(), Bounds::CompareYMax())->ymax / resolution;
			int const jmax = std::max_element(tile_bounds.begin(), tile_bounds.end(), Bounds::CompareXMax())->xmax / resolution;
			int const width = jmax - jmin + 1, height = imax - imin + 1;

			auto fill = Fill<5>(width, height);
			for (auto const &bounds: tile_bounds)
				fill(
					static_cast<int>(bounds.ymin / resolution) - imin,
					static_cast<int>(bounds.xmin / resolution) - jmin,
					static_cast<int>(bounds.ymax / resolution) - imin,
					static_cast<int>(bounds.xmax / resolution) - jmin
				);

			fill([&](auto i, auto j) {
				emplace_back((jmin + j + 0.5) * resolution, (imin + i + 0.5) * resolution, 0.0, 2, false, true, false);
			});
		}
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(PointIterator const &point) const { return std::hash<Point *>()(&*point); }
};

#endif
