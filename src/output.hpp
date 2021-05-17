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

	template <typename ...Args>
	auto static from(std::filesystem::path const &path, Args const &...args) {
		if (path == "-")
			return Variant(std::in_place_type_t<GeoJSON>(), path, args...);
		if (path.extension() == ".json")
			return Variant(std::in_place_type_t<GeoJSON>(), path, args...);
		if (path.extension() == ".shp")
			return Variant(std::in_place_type_t<Shapefile>(), path, args...);
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
		return std::visit([](auto const &output) -> bool { return output; }, variant);
	}

	template <typename ...Args>
	void operator()(Args const &...args) {
		std::visit([&](auto &output) { output(args...); }, variant);
	}
};

#endif
