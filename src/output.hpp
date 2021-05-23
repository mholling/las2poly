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

	auto static from(std::filesystem::path const &path) {
		if (path == "-")
			return Variant(std::in_place_type_t<GeoJSON>(), path);
		if (path.extension() == ".json")
			return Variant(std::in_place_type_t<GeoJSON>(), path);
		if (path.extension() == ".shp")
			return Variant(std::in_place_type_t<Shapefile>(), path);
		throw std::runtime_error("output file extension must be .json or .shp");
	}

	Variant variant;

public:
	Output(std::filesystem::path const &path) : variant(from(path)) { }

	auto ogc() const {
		return std::holds_alternative<GeoJSON>(variant);
	}

	operator bool() const {
		auto const exists = [](auto const &output) -> bool { return output; };
		return std::visit(exists, variant);
	}

	template <typename Polygons, typename OptionalSRS>
	void operator()(Polygons const &polygons, OptionalSRS const &srs) {
		std::visit([&](auto &output) { output(polygons, srs); }, variant);
	}
};

#endif
