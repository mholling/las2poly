#ifndef THIN_HPP
#define THIN_HPP

#include "record.hpp"
#include <utility>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <vector>

class Thin {
	using Indices = std::pair<std::uint32_t, std::uint32_t>;

	struct Cell : Indices {
		Cell(const Record &record, double resolution) : Indices(std::floor(record.x / resolution), std::floor(record.y / resolution)) { }
	};

	struct Hash {
		std::size_t operator()(const Cell &cell) const {
			if constexpr (sizeof(std::size_t) < 8) {
				auto constexpr hash = std::hash<std::uint32_t>();
				auto seed = hash(cell.first);
				return seed ^ (hash(cell.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
			} else
				return static_cast<std::size_t>(cell.first) << 32 | static_cast<std::size_t>(cell.second);
		}
	};

	using Records = std::unordered_map<Cell, Record, Hash>;
	using RecordIterator = typename Records::const_iterator;

	Records records;
	std::unordered_set<unsigned char> classes;
	double resolution;

	auto &insert(const Record &record) {
		auto cell = Cell(record, resolution);
		auto [existing, inserted] = records.emplace(cell, record);
		if (!inserted && record < existing->second) {
			records.erase(existing);
			records.emplace(cell, record);
		}
		return *this;
	}

	struct Iterator {
		const Thin &thin;
		RecordIterator here;

		Iterator(const Thin &thin, RecordIterator here) : thin(thin), here(here) { }
		auto &operator++() { ++here; return *this;}
		auto operator!=(Iterator other) const { return here != other.here; }
		auto &operator*() const { return here->second; }
	};

public:
	auto begin() const { return Iterator(*this, records.begin()); }
	auto   end() const { return Iterator(*this, records.end()); }

	auto size() const { return records.size(); }

	template <typename Classes>
	Thin(double resolution, Classes additional) : resolution(resolution), classes({2,3,4,5,6}) {
		classes.insert(additional.begin(), additional.end());
	}

	template <typename Tile>
	auto &operator+=(Tile tile) {
		for (const auto record: tile)
			if (classes.count(record.c))
				insert(record);
		return *this;
	}
};

#endif
