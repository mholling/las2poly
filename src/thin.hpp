#ifndef THIN_HPP
#define THIN_HPP

#include "raw_point.hpp"
#include "point.hpp"
#include <utility>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <vector>

class Thin {
	using Indices = std::pair<std::uint32_t, std::uint32_t>;

	struct Cell : Indices {
		Cell(const RawPoint &point, double resolution) : Indices(std::floor(point.x / resolution), std::floor(point.y / resolution)) { }
	};

	struct Hash {
		std::size_t operator()(const Cell &cell) const {
			if constexpr (sizeof(std::size_t) < 8) {
				auto constexpr hash = std::hash<std::uint32_t>();
				auto seed = hash(cell.first);
				return seed ^ (hash(cell.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
			} else
				return static_cast<std::size_t>(cell.first) << 32 | static_cast<std::size_t>(cell.second);
		}
	};

	std::unordered_map<Cell, RawPoint, Hash> points;
	std::unordered_set<unsigned char> classes;
	double resolution;

	auto &insert(const RawPoint &point) {
		auto cell = Cell(point, resolution);
		auto [existing, inserted] = points.emplace(cell, point);
		if (!inserted && point < existing->second) {
			points.erase(existing);
			points.emplace(cell, point);
		}
		return *this;
	}

public:
	template <typename Classes>
	Thin(double resolution, Classes additional) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(additional.begin(), additional.end());
	}

	template <typename Tile>
	auto &operator+=(Tile tile) {
		for (const auto point: tile)
			if (classes.count(point.c))
				insert(point);
		if (points.size() > UINT32_MAX)
			throw std::runtime_error("too many points");
		return *this;
	}

	auto operator()() {
		std::vector<Point> result;
		result.reserve(points.size());
		std::size_t index = 0;
		for (auto pair: points)
			result.emplace_back(pair.second, index++);
		return result;
	}
};

#endif
