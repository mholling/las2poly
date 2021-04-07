#ifndef TILE_HPP
#define TILE_HPP

#include "ply.hpp"
#include "las.hpp"
#include <variant>
#include <istream>
#include <array>
#include <stdexcept>
#include <cstddef>

class Tile {
	using TileVariant = std::variant<PLY, LAS>;

	static auto from(std::istream &input) {
		constexpr std::array<char, 4> las_magic = {'L','A','S','F'};
		constexpr std::array<char, 4> ply_magic = {'p','l','y','\n'};

		auto magic = std::array<char, 4>();
		input.read(magic.data(), sizeof(magic));

		if (magic == ply_magic)
			return TileVariant(PLY(input));
		else if (magic == las_magic)
			return TileVariant(LAS(input));
		else
			throw std::runtime_error("unrecognised file format");
	}

	struct Read {
		auto operator()(PLY &ply) { return ply.read(); }
		auto operator()(LAS &las) { return las.read(); }
	};

	struct Size {
		auto operator()(const PLY &ply) { return ply.size; }
		auto operator()(const LAS &las) { return las.size; }
	};

	auto read() { return std::visit(Read(), tile_variant); }

	TileVariant tile_variant;

	struct Iterator {
		Tile &tile;
		std::size_t index;

		Iterator(Tile &tile, std::size_t index) : tile(tile), index(index) { }
		auto &operator++() { ++index; return *this;}
		auto operator!=(const Iterator &other) const { return index != other.index; }
		auto operator*() const { return tile.read(); }
	};

public:
	Tile(std::istream &input) : tile_variant(from(input)) { }

	auto size() const { return std::visit(Size(), tile_variant); }

	auto begin() { return Iterator(*this, 0); }
	auto   end() { return Iterator(*this, size()); }
};

#endif
