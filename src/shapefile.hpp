////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SHAPEFILE_HPP
#define SHAPEFILE_HPP

#include "polygons.hpp"
#include "polygon.hpp"
#include "ring.hpp"
#include "vector.hpp"
#include <optional>
#include <filesystem>
#include <stdexcept>

class Shapefile {
	using EPSG = std::optional<int>;

	std::filesystem::path shp_path;
	std::filesystem::path shx_path;
	std::filesystem::path dbf_path;

public:
	Shapefile(std::filesystem::path const &shp_path, EPSG const &epsg) : shp_path(shp_path), shx_path(shp_path), dbf_path(shp_path) {
		if (epsg)
			throw std::runtime_error("can't store EPSG for shapefile output");
		shx_path.replace_extension(".shx");
		dbf_path.replace_extension(".dbf");
	}

	void operator()(Polygons const &polygons) {
		throw std::runtime_error("shapefile output not implemented");
	}
};

#endif
