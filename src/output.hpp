////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "geojson.hpp"
#include "shapefile.hpp"
#include <variant>
#include <filesystem>
#include <utility>
#include <stdexcept>

class Output {
	using Variant = std::variant<GeoJSON, Shapefile>;

	template <typename EPSG>
	auto static from(std::filesystem::path const &path, EPSG const &epsg) {
		if (path == "-")
			return Variant(std::in_place_type_t<GeoJSON>(), path, epsg);
		if (path.extension() == ".json")
			return Variant(std::in_place_type_t<GeoJSON>(), path, epsg);
		if (path.extension() == ".shp")
			return Variant(std::in_place_type_t<Shapefile>(), path, epsg);
		throw std::runtime_error("output file extension must be .json or .shp");
	}

	Variant variant;

public:
	template <typename ...Args>
	Output(Args const &...args) : variant(from(args...)) { }

	auto ogc() const {
		return std::holds_alternative<GeoJSON>(variant);
	}

	operator bool() const {
		auto const exists = [](auto const &output) -> bool { return output; };
		return std::visit(exists, variant);
	}

	template <typename Polygons>
	void operator()(Polygons const &polygons) {
		std::visit([&](auto &output) { output(polygons); }, variant);
	}
};

#endif
