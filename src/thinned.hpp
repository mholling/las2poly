#ifndef THINNED_HPP
#define THINNED_HPP

#include "point.hpp"
#include <utility>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <unordered_map>
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

	static auto better_than(const Point &point1, const Point &point2) {
		return point1.ground
			? point2.ground ? point1[2] < point2[2] : true
			: point2.ground ? false : point1[2] < point2[2];
	}

public:
	Thinned(double cell_size) : cell_size(cell_size) { }

	auto &insert(const Point &point) {
		auto pair = std::pair(Cell(point, cell_size), point);
		auto [existing, inserted] = thinned.insert(pair);
		if (!inserted && better_than(point, existing->second)) {
			thinned.erase(existing);
			thinned.insert(pair);
		}
		return *this;
	}

	template <typename Tile>
	auto &operator+=(Tile tile) {
		for (const auto point: tile)
			insert(point);
		return *this;
	}

	auto to_vector() {
		std::vector<Point> result;
		result.reserve(thinned.size());
		std::size_t index = 0;
		for (auto pair = thinned.begin(); pair != thinned.end(); )
			result.push_back(Point(std::move(thinned.extract(pair++).mapped()), index++));
		return result;
	}
};

#endif
