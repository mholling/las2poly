#ifndef CELL_HPP
#define CELL_HPP

#include "point.hpp"
#include <utility>
#include <cstdint>

struct Cell : std::pair<std::int32_t, std::int32_t> {
	Cell(const Point &point, double resolution) : pair(point[0] / resolution, point[1] / resolution) { }
};

#endif
