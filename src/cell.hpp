#ifndef CELL_HPP
#define CELL_HPP

#include "point.hpp"
#include <utility>
#include <cstdint>

struct Cell : std::pair<std::pair<std::int32_t, std::int32_t>, Point> {
	Cell(double resolution, const Point &point) : pair({{point[0] / resolution, point[1] / resolution}, point}) { }

	friend auto operator<(const Cell &cell1, const Cell &cell2) {
		return cell1.first < cell2.first;
	}

	friend auto operator>(const Cell &c1, const Cell &c2) {
		const auto &p1 = c1.second, &p2 = c2.second;
		return
			std::tuple(p1.key_point, 2 == p1.classification ? 2 : 8 == p1.classification ? 2 : 3 == p1.classification ? 1 : 0, p2.elevation) >
			std::tuple(p2.key_point, 2 == p2.classification ? 2 : 8 == p2.classification ? 2 : 3 == p2.classification ? 1 : 0, p1.elevation);
	}
};

#endif
