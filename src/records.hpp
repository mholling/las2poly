#ifndef RECORDS_HPP
#define RECORDS_HPP

#include "cell.hpp"
#include "point.hpp"
#include "tile.hpp"
#include <map>
#include <unordered_set>

class Records : std::map<Cell, Point> {
	double resolution;
	std::unordered_set<unsigned char> classes;

	auto &insert(const Point &point) {
		auto cell = Cell(point, resolution);
		auto [existing, inserted] = emplace(cell, point);
		if (!inserted && point > existing->second) {
			erase(existing);
			emplace(cell, point);
		}
		return *this;
	}

	struct Iterator {
		const_iterator here;

		Iterator(const_iterator here) : here(here) { }
		auto &operator++() { ++here; return *this;}
		auto operator!=(const Iterator &other) const { return here != other.here; }
		auto &operator*() const { return here->second; }
	};

public:
	auto begin() const { return Iterator(map::begin()); }
	auto   end() const { return Iterator(map::end()); }
	auto  size() const { return map::size(); }

	template <typename Classes>
	Records(double resolution, Classes additional) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(additional.begin(), additional.end());
	}

	auto &operator+=(Tile &&tile) {
		for (const auto point: tile)
			if (!point.withheld && (point.key_point || classes.count(point.classification)))
				insert(point);
		return *this;
	}

	auto &operator+=(Records &records) {
		merge(records);
		for (const auto &point: records)
			insert(point);
		return *this;
	}
};

#endif
