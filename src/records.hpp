#ifndef RECORDS_HPP
#define RECORDS_HPP

#include "cell.hpp"
#include "record.hpp"
#include "tile.hpp"
#include <unordered_map>
#include <unordered_set>

class Records : std::unordered_map<Cell, Record> {
	std::unordered_set<unsigned char> classes;
	double resolution;

	auto &insert(const Record &record) {
		auto cell = Cell(record, resolution);
		auto [existing, inserted] = emplace(cell, record);
		if (!inserted && record < existing->second) {
			erase(existing);
			emplace(cell, record);
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
	auto begin() const { return Iterator(unordered_map::begin()); }
	auto   end() const { return Iterator(unordered_map::end()); }
	auto  size() const { return unordered_map::size(); }

	template <typename Classes>
	Records(double resolution, Classes additional) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(additional.begin(), additional.end());
	}

	auto &operator+=(Tile &&tile) {
		for (const auto record: tile)
			if (classes.count(record.c))
				insert(record);
		return *this;
	}

	auto &operator+=(Records &records) {
		merge(records);
		for (const auto record: records)
			if (classes.count(record.c))
				insert(record);
		return *this;
	}
};

#endif
