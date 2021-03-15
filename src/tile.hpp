#ifndef TILE_HPP
#define TILE_HPP

#include "ply.hpp"
#include "las.hpp"
#include <variant>
#include <istream>
#include <array>
#include <stdexcept>
#include <cstddef>
#include <string>

class Tile {
	using TileVariant = std::variant<PLY, LAS>;

	static auto from(std::istream &input) {
		constexpr std::array<char, 4> las_magic = {'L','A','S','F'};
		constexpr std::array<char, 4> ply_magic = {'p','l','y','\n'};

		std::array<char, 4> magic;
		input.read(magic.data(), sizeof(magic));

		if (magic == ply_magic)
			return TileVariant(PLY(input));
		else if (magic == las_magic)
			return TileVariant(LAS(input));
		else
			throw std::runtime_error("unrecognised file format");
	}

	struct Record {
		auto operator()(PLY &ply) { return ply.record(); }
		auto operator()(LAS &las) { return las.record(); }
	};

	struct Count {
		auto operator()(PLY &ply) { return ply.count; }
		auto operator()(LAS &las) { return las.count; }
	};

	auto record() { return std::visit(Record(), tile_variant); }
	auto count() { return std::visit(Count(), tile_variant); }

	std::istream &input;
	TileVariant tile_variant;

	struct Iterator {
		Tile &tile;
		std::size_t index;

		Iterator(Tile &tile, std::size_t index) : tile(tile), index(index) { }
		auto &operator++() { ++index; return *this;}
		auto operator!=(Iterator other) const { return index != other.index; }
		auto operator*() const { return tile.record(); }
	};

public:
	Tile(std::istream &input) : input(input), tile_variant(from(input)) { }

	auto begin() { return Iterator(*this, 0); }
	auto   end() { return Iterator(*this, count()); }
};

#endif
