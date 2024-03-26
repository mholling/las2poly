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
#include "app.hpp"
#include "thin.hpp"
#include "fill.hpp"
#include "tile.hpp"
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <set>
#include <iostream>
#include <mutex>
#include <exception>
#include <stdexcept>
#include <thread>
#include <cmath>
#include <numeric>
#include <cstddef>

class Points : public std::vector<Point> {
	using Path = std::filesystem::path;
	using Paths = std::vector<Path>;
	using PathIterator = Paths::const_iterator;
	using Discard = std::unordered_set<unsigned char>;

	std::vector<Bounds> tile_bounds;
	std::set<OptionalSRS> distinct_srs;

	void load(App const &app, Path const &path, Thin const &thin) {
		try {
			auto input = std::ifstream(path, std::ios::binary);
			thin(app, *this, input);
		} catch (std::ios_base::failure &) {
			throw std::runtime_error(path.string() + ": problem reading file");
		} catch (std::runtime_error &error) {
			throw std::runtime_error(path.string() + ": " + error.what());
		}
	}

	void load(App const &app, PathIterator begin, PathIterator end, Thin const &thin, std::mutex &mutex, std::exception_ptr &exception, int threads) {
		if (auto lock = std::lock_guard(mutex); exception)
			return;
		try {
			if (begin + 1 == end) {
				auto const &path = *begin;
				load(app, path, thin);
			} else {
				auto const middle = begin + (end - begin) / 2;
				auto points1 = Points();
				auto points2 = Points();
				if (1 == threads) {
					points1.load(app, begin, middle, thin, mutex, exception, 1);
					points2.load(app, middle, end, thin, mutex, exception, 1);
				} else {
					auto thread1 = std::thread([&]() {
						points1.load(app, begin, middle, thin, mutex, exception, threads/2);
					}), thread2 = std::thread([&]() {
						points2.load(app, middle, end, thin, mutex, exception, threads - threads/2);
					});
					thread1.join(), thread2.join();
				}
				thin(*this, points1, points2);
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

	Points() = default;

public:
	Points(App const &app, Path const &path) {
		auto const thin = Thin();
		load(app, path, thin);
	}

	Points(App const &app) {
		auto const resolution = *app.width / std::sqrt(8.0);
		auto const thin = Thin(resolution);
		auto mutex = std::mutex();
		auto exception = std::exception_ptr();

		app.log("reading", app.tile_paths.size(), "tile");
		load(app, app.tile_paths.begin(), app.tile_paths.end(), thin, mutex, exception, app.io_threads);

		if (exception)
			std::rethrow_exception(exception);

		if (!app.land && size() > 2) {
			app.log("synthesising extra points");
			auto fill = Fill(tile_bounds, resolution);
			fill(*this);
		}
	}

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

	auto srs() const {
		return distinct_srs.empty() ? OptionalSRS() : *distinct_srs.begin();
	}
};

using PointIterator = Points::iterator;

template <>
Bounds::Bounds(PointIterator const &point) : Bounds(*point) { }

template <> struct std::hash<PointIterator> {
	std::size_t operator()(PointIterator const &point) const { return std::hash<Point *>()(&*point); }
};

#endif
