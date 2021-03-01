#ifndef THINNED_HPP
#define THINNED_HPP

#include "thinned.hpp"
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

class Thinned {
	using Indices = std::pair<std::uint32_t, std::uint32_t>;

	struct Cell : Indices {
		Cell(const Point &point, double cell_size) : Indices(std::floor(point[0] / cell_size), std::floor(point[1] / cell_size)) { }
	};

	struct CellHash {
		std::size_t operator()(const Cell &cell) const { return static_cast<std::size_t>(cell.first) | static_cast<std::size_t>(cell.second) << 32; }
	};

	std::unordered_map<Cell, Point, CellHash> thinned;
	double cell_size;

public:
	Thinned(double cell_size) : cell_size(cell_size) { }

	template <typename Function>
	auto &insert(const Point &point, Function better_than) {
		auto pair = std::pair(Cell(point, cell_size), point);
		auto [existing, inserted] = thinned.insert(pair);
		if (!inserted && better_than(point, existing->second)) {
			thinned.erase(existing);
			thinned.insert(pair);
		}
		return *this;
	}

	std::vector<Point> to_vector() {
		std::vector<Point> result;
		result.reserve(thinned.size());
		std::size_t index = 0;
		for (auto pair = thinned.begin(); pair != thinned.end(); )
			result.push_back(Point(std::move(thinned.extract(pair++).mapped()), index++));
		return result;
	}
};

#endif
