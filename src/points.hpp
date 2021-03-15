#ifndef POINTS_HPP
#define POINTS_HPP

#include "point.hpp"
#include "queue.hpp"
#include "thin.hpp"
#include "tile.hpp"
#include <vector>
#include <string>
#include <thread>
#include <mutex>

struct Points : std::vector<Point> {
	Points(const std::vector<std::string> &tile_paths, double resolution, const std::vector<int> &classes, int thread_count) {
		auto paths = Queue<std::string>();
		auto threads = std::vector<std::thread>();
		auto mutex = std::mutex();
		auto records = Thin(resolution, classes);

		while (thread_count--)
			threads.emplace_back([&]() {
				for (std::string path; paths >> path; ) {
					auto tile = Thin(resolution, classes);
					if (path == "-")
						tile += Tile(std::cin);
					else {
						std::ifstream input(path, std::ios::binary);
						input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						tile += Tile(input);
					}
					std::lock_guard lock(mutex);
					records += tile;
				}
			});
		for (const auto &path: tile_paths)
			paths << path;
		paths.close();
		for (auto &thread: threads)
			thread.join();
		reserve(records.size());
		std::size_t index = 0;
		for (const auto &record: records)
			emplace_back(record, index++);
	}
};

#endif
