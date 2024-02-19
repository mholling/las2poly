////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "geojson.hpp"
#include "shapefile.hpp"
#include "app.hpp"
#include <variant>
#include <filesystem>
#include <utility>
#include <stdexcept>

class Output {
	using Variant = std::variant<GeoJSON, Shapefile>;
	Variant variant;

	auto static from(App const &app) {
		if (app.path && app.path->extension() == ".shp")
			return Variant(std::in_place_type_t<Shapefile>(), *app.path);
		else
			return Variant(std::in_place_type_t<GeoJSON>(), app.path);
	}

public:
	Output(App const &app) : variant(from(app)) {
		if (!app.overwrite && *this)
			throw std::runtime_error("output file already exists");
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
