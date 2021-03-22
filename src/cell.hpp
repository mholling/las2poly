#ifndef CELL_HPP
#define CELL_HPP

#include "record.hpp"
#include <utility>
#include <cstdint>
#include <cmath>
#include <functional>
#include <cstddef>

struct Cell : std::pair<std::int32_t, std::int32_t> {
	Cell(const Record &record, double resolution) : pair(std::floor(record.x / resolution), std::floor(record.y / resolution)) { }
};

template <> struct std::hash<Cell> {
	std::size_t operator()(const Cell &cell) const {
		if constexpr (sizeof(std::size_t) < 8) {
			auto constexpr hash = std::hash<std::uint32_t>();
			auto seed = hash(cell.first);
			return seed ^ (hash(cell.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		} else
			return static_cast<std::size_t>(cell.first) << 32 | static_cast<std::size_t>(cell.second);
	}
};

#endif
