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

public:
	template <typename Discard>
	Points(Paths const &tile_paths, double resolution, Discard const &discard, bool water, unsigned threads) {
		auto points = Load(resolution, discard)(tile_paths, threads);

		if (water) {
			auto landfill = Points();
			auto incoming = std::set<Bounds, Bounds::CompareMin>();
			auto outgoing = std::set<Bounds, Bounds::CompareMax>();

			for (auto const &bounds: points.tile_bounds) {
				incoming.insert(bounds);
				outgoing.insert(bounds);
			}

			auto [xmin, ymin] = incoming.begin()->min();
			auto [xmax, ymax] = (--outgoing.end())->max();

			incoming.emplace(xmin - 5 * resolution, ymin - 5 * resolution, xmax + 5 * resolution, ymin - 5 * resolution);
			outgoing.emplace(xmin - 5 * resolution, ymin - 5 * resolution, xmax + 5 * resolution, ymin - 5 * resolution);
			incoming.emplace(xmin - 5 * resolution, ymax + 5 * resolution, xmax + 5 * resolution, ymax + 5 * resolution);
			outgoing.emplace(xmin - 5 * resolution, ymax + 5 * resolution, xmax + 5 * resolution, ymax + 5 * resolution);

			auto current = std::set<Bounds, Bounds::CompareYMax>();
			double x0 = incoming.begin()->xmin, x1;
			for (auto in = incoming.begin(), in_end = incoming.end(), out = outgoing.begin(), out_end = outgoing.end(); out != out_end; x0 = x1) {
				if (in != in_end && in->min() < out->max())
					current.insert(*in++);
				else
					current.erase(*out++);
				if (out == out_end)
					break;
				if (in != in_end && in->min() < out->max())
					x1 = in->xmin;
				else
					x1 = out->xmax;
				auto const m0 = static_cast<int>(x0 / resolution);
				auto const m1 = static_cast<int>(x1 / resolution) + 1;
				for (auto bounds = current.begin(), bounds_end = current.end(); bounds != bounds_end; ) {
					auto const n0 = static_cast<int>(bounds->ymax / resolution) + 1;
					bounds = std::min_element(++bounds, bounds_end, Bounds::CompareYMin());
					if (bounds == bounds_end)
						break;
					auto const n1 = static_cast<int>(bounds->ymin / resolution);
					for (int n = n0; n < n1; ++n)
						for (int m = m0; m < m1; ++m)
							landfill.emplace_back((m + 0.5) * resolution, (n + 0.5) * resolution, 0.0, 2, false, true, false);
				}
			}

			std::sort(landfill.begin(), landfill.end(), [](auto const &p1, auto const &p2) {
				return p1[0] < p2[0] ? true : p1[0] > p2[0] ? false : p1[1] < p2[1];
			});
			landfill.erase(std::unique(landfill.begin(), landfill.end(), [](auto const &p1, auto const &p2) {
				return p1[0] == p2[0] && p1[1] == p2[1];
			}), landfill.end());
			points.insert(points.end(), landfill.begin(), landfill.end());
		}

		swap(points);
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(PointIterator const &point) const { return std::hash<Point *>()(&*point); }
};

#endif
