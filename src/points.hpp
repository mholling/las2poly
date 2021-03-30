#ifndef POINTS_HPP
#define POINTS_HPP

#include "point.hpp"
#include "thin.hpp"
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

	Thin thin;
	std::unordered_set<unsigned char> classes;
	std::mutex mutex;
	std::exception_ptr exception;

	auto load(PathIterator begin, PathIterator end, int threads) {
		for (auto lock = std::lock_guard(mutex); exception; )
			return thin();
		const auto middle = begin + (end - begin) / 2;
		if (begin + 1 == end)
			try {
				const auto &path = *begin;
				try {
					if (path == "-") {
						std::cin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						return thin(Tile(std::cin), classes);
					} else {
						std::ifstream input(path, std::ios::binary);
						input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						return thin(Tile(input), classes);
					}
				} catch (std::ios_base::failure &) {
					throw std::runtime_error(path + ": problem reading file");
				} catch (std::runtime_error &error) {
					throw std::runtime_error(path + ": " + error.what());
				}
			} catch (std::runtime_error &) {
				std::lock_guard lock(mutex);
				exception = std::current_exception();
				return thin();
			}
		else if (1 == threads)
			return thin(load(begin, middle, 1), load(middle, end, 1));
		else {
			auto points1 = thin();
			auto points2 = thin();
			auto thread1 = std::thread([&]() {
				points1 = load(begin, middle, threads/2);
			}), thread2 = std::thread([&]() {
				points2 = load(middle, end, threads - threads/2);
			});
			thread1.join(), thread2.join();
			return thin(points1, points2);
		}
	}

public:
	template <typename Classes>
	Points(const Paths &tile_paths, double resolution, const Classes &additional_classes, int threads) : thin(resolution), classes({2,3,4,5,6,10,11,17}) {
		classes.insert(additional_classes.begin(), additional_classes.end());
		auto points = load(tile_paths.begin(), tile_paths.end(), threads);
		if (exception)
			std::rethrow_exception(exception);
		swap(points);
	}
};

using PointIterator = Points::iterator;

template <> struct std::hash<PointIterator> {
	std::size_t operator()(const PointIterator &point) const { return std::hash<Point *>()(&*point); }
};

#endif
