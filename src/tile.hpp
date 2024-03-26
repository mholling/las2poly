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
#include "srs.hpp"
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
		std::array<char, 4> magic;

		input.exceptions(std::istream::failbit | std::istream::badbit);
		input.read(magic.data(), magic.size());

		if (magic == ply_magic)
			return Variant(std::in_place_type<PLY>, input);
		if (magic == las_magic)
			return Variant(std::in_place_type<LAS>, input);
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

		Iterator(Tile &tile, std::size_t index) :
			tile(tile),
			index(index)
		{ }

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

	struct GetSRS {
		auto const &operator()(PLY &ply) const { return ply.srs; }
		auto const &operator()(LAS &las) const { return las.srs; }
	};

public:
	Bounds bounds;

	Tile(std::istream &input) : variant(from(input)) { }

	auto size() const {
		auto const size = [](auto const &tile) { return tile.size; };
		return std::visit(size, variant);
	}

	auto srs() {
		return std::visit(GetSRS(), variant);
	}

	auto begin() { return Iterator(*this, 0); }
	auto   end() { return Iterator(*this, size()); }
};

#endif
