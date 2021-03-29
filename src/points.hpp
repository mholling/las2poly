#ifndef POINTS_HPP
#define POINTS_HPP

#include "point.hpp"
#include "cells.hpp"
#include "tile.hpp"
#include <vector>
#include <string>
#include <unordered_set>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <thread>
#include <functional>
#include <cstddef>

class Points : public std::vector<Point> {
	using Paths = std::vector<std::string>;
	using PathIterator = Paths::const_iterator;
	using Classes = std::unordered_set<unsigned char>;

	double resolution;
	Classes classes;
	std::mutex mutex;
	std::exception_ptr exception;

	auto load_cells(PathIterator begin, PathIterator end, int threads) {
		for (auto lock = std::lock_guard(mutex); exception; )
			return Cells();
		const auto middle = begin + (end - begin) / 2;
		if (begin + 1 == end)
			try {
				const auto &path = *begin;
				try {
					if (path == "-") {
						std::cin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						return Cells(Tile(std::cin), resolution, classes);
					} else {
						std::ifstream input(path, std::ios::binary);
						input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						return Cells(Tile(input), resolution, classes);
					}
				} catch (std::ios_base::failure &) {
					throw std::runtime_error(path + ": problem reading file");
				} catch (std::runtime_error &error) {
					throw std::runtime_error(path + ": " + error.what());
				}
			} catch (std::runtime_error &) {
				std::lock_guard lock(mutex);
				exception = std::current_exception();
				return Cells();
			}
		else if (1 == threads)
			return load_cells(begin, middle, 1) + load_cells(middle, end, 1);
		else {
			auto cells1 = Cells();
			auto cells2 = Cells();
			auto thread1 = std::thread([&]() {
				cells1 = load_cells(begin, middle, threads/2);
			}), thread2 = std::thread([&]() {
				cells2 = load_cells(middle, end, threads - threads/2);
			});
			thread1.join(), thread2.join();
			return cells1 + cells2;
		}
	}

public:
	Points(const Paths &tile_paths, double resolution, const std::vector<int> &additional_classes, int threads) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(additional_classes.begin(), additional_classes.end());
		auto cells = load_cells(tile_paths.begin(), tile_paths.end(), threads);
		if (exception)
			std::rethrow_exception(exception);
		reserve(cells.size());
		for (const auto &[indices, point]: cells)
			push_back(point);
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(const PointIterator &point) const { return std::hash<Point *>()(&*point); }
};

#endif
