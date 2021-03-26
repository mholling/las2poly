#ifndef CELL_HPP
#define CELL_HPP

#include "point.hpp"
#include <utility>
#include <cstdint>
#include <cmath>

struct Cell : std::pair<std::int32_t, std::int32_t> {
	Cell(const Point &point, double resolution) : pair(std::floor(point[0] / resolution), std::floor(point[1] / resolution)) { }
};

#endif
