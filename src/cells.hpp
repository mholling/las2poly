#ifndef CELLS_HPP
#define CELLS_HPP

#include "cell.hpp"
#include "tile.hpp"
#include <vector>
#include <algorithm>
#include <functional>

struct Cells : std::vector<Cell> {
	Cells() = default;

	template <typename Classes>
	Cells(Tile &&tile, double resolution, const Classes &classes) {
		reserve(tile.size());
		for (const auto point: tile)
			if (!point.withheld && (point.key_point || classes.count(point.classification)))
				emplace_back(resolution, point);
		std::sort(begin(), end());

		auto here = begin(), cells_end = end();
		for (auto range_begin = begin(); range_begin != cells_end; ++here) {
			auto range_end = std::upper_bound(range_begin, cells_end, *range_begin);
			*here = *std::min_element(range_begin, range_end, std::greater());
			range_begin = range_end;
		}
		erase(here, cells_end);
	}

	friend auto operator+(const Cells &cells1, const Cells &cells2) {
		auto cells = Cells();
		cells.reserve(cells1.size() + cells2.size());
		for (auto here1 = cells1.begin(), here2 = cells2.begin(), end1 = cells1.end(), end2 = cells2.end(); here1 != end1 || here2 != end2; ) {
			for (; here1 != end1 && (here2 == end2 || *here1 < *here2); ++here1)
				cells.push_back(*here1);
			for (; here2 != end2 && (here1 == end1 || *here2 < *here1); ++here2)
				cells.push_back(*here2);
			if (here1 != end1 && here2 != end2)
				cells.push_back(*here1 > *here2 ? *here1 : *here2);
			if (here1 != end1) ++here1;
			if (here2 != end2) ++here2;
		}
		return cells;
	}
};

#endif
