#ifndef CELL_HPP
#define CELL_HPP

#include "record.hpp"
#include <utility>
#include <cstdint>
#include <cmath>

struct Cell : std::pair<std::int32_t, std::int32_t> {
	Cell(const Record &record, double resolution) : pair(std::floor(record.x / resolution), std::floor(record.y / resolution)) { }
};

#endif
