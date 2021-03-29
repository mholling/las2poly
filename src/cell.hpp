#ifndef CELL_HPP
#define CELL_HPP

#include "point.hpp"
#include <utility>
#include <cstdint>

struct Cell : std::pair<std::pair<std::int32_t, std::int32_t>, Point> {
	Cell(const Point &point, double resolution) : pair({{point[0] / resolution, point[1] / resolution}, point}) { }

	friend auto operator<(const Cell &cell1, const Cell &cell2) {
		return cell1.first < cell2.first;
	}
};

#endif
