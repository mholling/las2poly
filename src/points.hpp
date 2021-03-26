#ifndef POINTS_HPP
#define POINTS_HPP

#include "point.hpp"
#include "queue.hpp"
#include "thinned.hpp"
#include "tile.hpp"
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <functional>
#include <cstddef>

struct Points : std::vector<Point> {
	Points(const std::vector<std::string> &tile_paths, double resolution, const std::vector<int> &classes, int thread_count) {
		auto paths = Queue<std::string>();
		auto threads = std::vector<std::thread>();
		auto mutex = std::mutex();
		auto thinned = Thinned(resolution, classes);
		auto exception = std::exception_ptr();

		while (thread_count--)
			threads.emplace_back([&]() {
				for (std::string path; paths >> path; )
					try {
						auto tile = Thinned(resolution, classes);
						try {
							if (path == "-") {
								std::cin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
								tile += Tile(std::cin);
							} else {
								std::ifstream input(path, std::ios::binary);
								input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
								tile += Tile(input);
							}
						} catch (std::ios_base::failure &) {
							throw std::runtime_error(path + ": problem reading file");
						} catch (std::runtime_error &error) {
							throw std::runtime_error(path + ": " + error.what());
						}
						std::lock_guard lock(mutex);
						if (exception)
							break;
						thinned += tile;
					} catch (std::runtime_error &) {
						std::lock_guard lock(mutex);
						exception = std::current_exception();
						break;
					}
			});

		for (const auto &path: tile_paths)
			paths << path;
		paths.close();
		for (auto &thread: threads)
			thread.join();
		if (exception)
			std::rethrow_exception(exception);
		reserve(thinned.size());
		for (const auto &[cell, point]: thinned)
			push_back(point);
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(const PointIterator &point) const { return std::hash<Point *>()(&*point); }
};

#endif
