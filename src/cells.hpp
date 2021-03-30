#ifndef CELLS_HPP
#define CELLS_HPP

#include "cell.hpp"
#include "tile.hpp"
#include <vector>
#include <unordered_set>
#include <utility>
#include <iterator>
#include <cstddef>
#include <algorithm>

class Cells : public std::vector<Cell> {
	using CellIterator = const_iterator;
	using Classes = std::unordered_set<unsigned char>;

	struct ComparePointQuality {
		auto operator()(const Cell &c1, const Cell &c2) {
			const auto &p1 = c1.second, &p2 = c2.second;
			return
				std::tuple(p1.key_point, 2 == p1.classification ? 2 : 8 == p1.classification ? 2 : 3 == p1.classification ? 1 : 0, p2.elevation) <
				std::tuple(p2.key_point, 2 == p2.classification ? 2 : 8 == p2.classification ? 2 : 3 == p2.classification ? 1 : 0, p1.elevation);
		}
	};

	struct Iterator {
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = Cell;
		using pointer           = Cell*;
		using reference         = Cell&;

		CellIterator cells_end, range_begin, range_end, range_best;

		Iterator(const CellIterator &range_begin, const CellIterator &cells_end) : cells_end(cells_end), range_begin(range_begin) {
			update();
		}

		void update() {
			if (range_begin == cells_end)
				return;
			range_end = std::upper_bound(range_begin, cells_end, *range_begin);
			range_best = std::max_element(range_begin, range_end, ComparePointQuality());
		}

		auto &operator++() {
			range_begin = range_end;
			update();
			return *this;
		}

		auto operator!=(const Iterator &other) const {
			return range_begin != other.range_begin;
		}

		auto operator==(const Iterator &other) const {
			return range_begin == other.range_begin;
		}

		auto &operator*() const {
			return *range_best;
		}
	};

public:
	auto begin() const { return Iterator(vector::begin(), vector::end()); }
	auto   end() const { return Iterator(vector::end(), vector::end()); }

	Cells() = default;

	Cells(Tile &&tile, double resolution, const Classes &classes) {
		reserve(tile.size());
		for (const auto point: tile)
			if (!point.withheld && (point.key_point || classes.count(point.classification)))
				emplace_back(point, resolution);
		std::sort(vector::begin(), vector::end());
	}

	friend auto operator+(const Cells &cells1, const Cells &cells2) {
		auto cells = Cells();
		cells.reserve(cells1.size() + cells2.size());
		std::merge(cells1.begin(), cells1.end(), cells2.begin(), cells2.end(), std::back_inserter(cells));
		return cells;
	}
};

#endif
