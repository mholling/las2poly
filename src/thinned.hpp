#ifndef THINNED_HPP
#define THINNED_HPP

#include "cell.hpp"
#include "point.hpp"
#include "tile.hpp"
#include <map>
#include <unordered_set>

class Thinned : public std::map<Cell, Point> {
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

public:
	template <typename Classes>
	Thinned(double resolution, Classes additional) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(additional.begin(), additional.end());
	}

	auto &operator+=(Tile &&tile) {
		for (const auto point: tile)
			if (!point.withheld && (point.key_point || classes.count(point.classification)))
				insert(point);
		return *this;
	}

	auto &operator+=(Thinned &thinned) {
		merge(thinned);
		for (const auto &[cell, point]: thinned)
			insert(point);
		return *this;
	}
};

#endif
