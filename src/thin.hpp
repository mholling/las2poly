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

	struct CellHash {
		std::size_t operator()(const Cell &cell) const { return static_cast<std::size_t>(cell.first) | static_cast<std::size_t>(cell.second) << 32; }
	};

	std::unordered_map<Cell, RawPoint, CellHash> thinned;
	std::unordered_set<unsigned char> classes;
	double resolution;

	auto &insert(const RawPoint &point) {
		auto cell = Cell(point, resolution);
		auto [existing, inserted] = thinned.emplace(cell, point);
		if (!inserted && point < existing->second) {
			thinned.erase(existing);
			thinned.emplace(cell, point);
		}
		return *this;
	}

public:
	template <typename Extra>
	Thin(double resolution, Extra extra) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(extra.begin(), extra.end());
	}

	template <typename Tile>
	auto &operator+=(Tile tile) {
		for (const auto point: tile)
			if (classes.count(point.c))
				insert(point);
		if (thinned.size() > UINT32_MAX)
			throw std::runtime_error("too many points");
		return *this;
	}

	auto operator()() {
		std::vector<Point> result;
		result.reserve(thinned.size());
		std::size_t index = 0;
		for (auto pair: thinned)
			result.emplace_back(pair.second, index++);
		return result;
	}
};

#endif
