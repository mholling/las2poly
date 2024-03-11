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
#include "polygons.hpp"
#include "points.hpp"
#include <variant>
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

	template <typename ...Args>
	void operator()(Args const &...args) {
		std::visit([&](auto &output) { output(args...); }, variant);
	}

	operator bool() const {
		auto const exists = [](auto const &output) -> bool { return output; };
		return std::visit(exists, variant);
	}

	auto allow_self_intersections() const {
		auto const allow = [](auto const &output) { return output.allow_self_intersections; };
		return std::visit(allow, variant);
	}

public:
	Output(App const &app) : variant(from(app)) {
		if (!app.overwrite && *this)
			throw std::runtime_error("output file already exists");
	}

	Output(App const &app, Polygons const &polygons, Points const &points) : Output(app) {
		auto const polys = polygons.reassemble(app, allow_self_intersections());
		app.log("saving", polys.size(), "polygon");
		if (app.multi && app.lines)
			(*this)(polys.multilinestrings(), points.srs());
		else if (app.lines)
			(*this)(polys.linestrings(), points.srs());
		else if (app.multi)
			(*this)(polys.multipolygon(), points.srs());
		else
			(*this)(polys, points.srs());
	}
};

#endif
