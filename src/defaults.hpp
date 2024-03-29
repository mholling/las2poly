////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef DEFAULTS_HPP
#define DEFAULTS_HPP

#include "app.hpp"
#include "points.hpp"
#include "mesh.hpp"
#include <vector>
#include <filesystem>
#include <mutex>
#include <exception>
#include <thread>
#include <algorithm>
#include <iomanip>

class Defaults {
	std::vector<double> medians;

	using Paths = std::vector<std::filesystem::path>;
	using PathIterator = Paths::const_iterator;

	Defaults() = default;

	void load(App const &app, PathIterator begin, PathIterator end, std::mutex &mutex, std::exception_ptr &exception, int threads) {
		if (auto lock = std::lock_guard(mutex); exception)
			return;
		try {
			if (begin + 1 == end) {
				auto points = Points(app, *begin);
				auto mesh = Mesh(points);
				if (auto const median = mesh.median_length(); median)
					medians.push_back(*median);
			} else {
				auto const middle = begin + (end - begin) / 2;
				auto defaults1 = Defaults();
				auto defaults2 = Defaults();
				if (1 == threads) {
					defaults1.load(app, begin, middle, mutex, exception, 1);
					defaults2.load(app, middle, end, mutex, exception, 1);
				} else {
					auto thread1 = std::thread([&]() {
						defaults1.load(app, begin, middle, mutex, exception, threads/2);
					}), thread2 = std::thread([&]() {
						defaults2.load(app, middle, end, mutex, exception, threads - threads/2);
					});
					thread1.join(), thread2.join();
				}
				medians.insert(medians.end(), defaults1.medians.begin(), defaults1.medians.end());
				medians.insert(medians.end(), defaults2.medians.begin(), defaults2.medians.end());
			}
		} catch (std::runtime_error &) {
			auto lock = std::lock_guard(mutex);
			exception = std::current_exception();
		}
	}

public:
	Defaults(App &app) {
		if (!app.width) {
			auto mutex = std::mutex();
			auto exception = std::exception_ptr();

			app.log("reading", app.tile_paths.size(), "tile");
			load(app, app.tile_paths.begin(), app.tile_paths.end(), mutex, exception, app.io_threads);

			if (exception)
				std::rethrow_exception(exception);

			auto const begin = medians.begin(), end = medians.end();
			auto const median = begin + (end - begin) / 2;
			std::nth_element(begin, median, end);

			if (median == end)
				throw std::runtime_error("not enough points");

			app.width = 4 * *median;
			app.log("using ", std::setprecision(1), *app.width, "m minimum width");
		}

		if (!app.area)
			app.area = 4 * *app.width * *app.width;

		if (!app.scale)
			app.scale = *app.width;
	}
};

#endif
