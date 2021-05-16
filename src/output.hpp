////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "geojson.hpp"
#include <variant>
#include <string>
#include <utility>
// #include <filesystem>

class Output {
	using Variant = std::variant<GeoJSON>;

	template <typename ...Args>
	auto static from(std::string const &path, Args const &...args) {
		// TODO: detect output variant from path extension
		return Variant(std::in_place_type_t<GeoJSON>(), path, args...);
	}

	Variant variant;

public:
	template <typename ...Args>
	Output(Args const &...args) : variant(from(args...)) { }

	template <typename ...Args>
	void operator()(Args const &...args) {
		std::visit([&](auto &output) { output(args...); }, variant);
	}
};

#endif
