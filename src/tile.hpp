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
#include <utility>
#include <stdexcept>
#include <cstddef>

class Tile {
	using Variant = std::variant<PLY, LAS>;

	auto static from(std::istream &input) {
		std::array<char, 4> static constexpr las_magic = {'L','A','S','F'};
		std::array<char, 4> static constexpr ply_magic = {'p','l','y','\n'};

		auto magic = std::array<char, 4>();
		input.read(magic.data(), magic.size());

		if (magic == ply_magic)
			return Variant(std::in_place_type_t<PLY>(), input);
		if (magic == las_magic)
			return Variant(std::in_place_type_t<LAS>(), input);
		throw std::runtime_error("unrecognised file format");
	}

	auto read() {
		auto const read = [](auto &tile) { return tile.read(); };
		auto const point = std::visit(read, variant);
		bounds += Bounds(point);
		return point;
	}

	Variant variant;

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

	Tile(std::istream &input) : variant(from(input)) { }

	auto size() const {
		auto const size = [](auto const &tile) { return tile.size; };
		return std::visit(size, variant);
	}

	auto epsg() const {
		auto const epsg = [](auto const &tile) { return tile.epsg; };
		return std::visit(epsg, variant);
	}

	auto begin() { return Iterator(*this, 0); }
	auto   end() { return Iterator(*this, size()); }
};

#endif
