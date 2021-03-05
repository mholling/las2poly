#ifndef THINNED_HPP
#define THINNED_HPP

#include "raw_point.hpp"
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
		Cell(const RawPoint &point, double cell_size) : Indices(std::floor(point.x / cell_size), std::floor(point.y / cell_size)) { }
	};

	struct CellHash {
		std::size_t operator()(const Cell &cell) const { return static_cast<std::size_t>(cell.first) | static_cast<std::size_t>(cell.second) << 32; }
	};

	std::unordered_map<Cell, RawPoint, CellHash> thinned;
	double cell_size;

	static auto better_than(const RawPoint &point1, const RawPoint &point2) {
		return point1.ground()
			? point2.ground() ? point1.z < point2.z : true
			: point2.ground() ? false : point1.z < point2.z;
	}

	auto &insert(const RawPoint &point) {
		auto pair = std::pair(Cell(point, cell_size), point);
		auto [existing, inserted] = thinned.insert(pair);
		if (!inserted && better_than(point, existing->second)) {
			thinned.erase(existing);
			thinned.insert(pair);
		}
		return *this;
	}

public:
	Thinned(double cell_size) : cell_size(cell_size) { }

	template <typename Tile>
	auto &operator+=(Tile tile) {
		for (const auto point: tile)
			insert(point);
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
