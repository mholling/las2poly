////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef TILE_HPP
#define TILE_HPP

#include "ply.hpp"
#include "las.hpp"
#include "bounds.hpp"
#include <variant>
#include <istream>
#include <array>
#include <stdexcept>
#include <cstddef>

class Tile {
	using TileVariant = std::variant<PLY, LAS>;

	auto static from(std::istream &input) {
		std::array<char, 4> static constexpr las_magic = {'L','A','S','F'};
		std::array<char, 4> static constexpr ply_magic = {'p','l','y','\n'};

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
		auto operator()(PLY &ply) const { return ply.read(); }
		auto operator()(LAS &las) const { return las.read(); }
	};

	struct Size {
		auto operator()(PLY const &ply) const { return ply.size; }
		auto operator()(LAS const &las) const { return las.size; }
	};

	auto read() {
		auto point = std::visit(Read(), tile_variant);
		bounds += Bounds(point);
		return point;
	}

	TileVariant tile_variant;

	struct Iterator {
		Tile &tile;
		std::size_t index;

		Iterator(Tile &tile, std::size_t index) : tile(tile), index(index) { }

		auto &operator++() {
			++index;
			return *this;
		}

		auto operator!=(Iterator const &other) const {
			return index != other.index;
		}

		auto operator*() const {
			return tile.read();
		}
	};

public:
	Bounds bounds;

	Tile(std::istream &input) : tile_variant(from(input)) { }

	auto size() const { return std::visit(Size(), tile_variant); }

	auto begin() { return Iterator(*this, 0); }
	auto   end() { return Iterator(*this, size()); }
};

#endif
