#include "args.hpp"
#include "thinned.hpp"
#include "ply.hpp"
#include "triangulate.hpp"
#include "polygon.hpp"
#include <optional>
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <fstream>
#include <utility>
#include <iostream>

int main(int argc, char *argv[]) {
	try {
		std::optional<double> length = 10.0;
		std::optional<double> width;
		std::optional<double> height = 5.0;
		std::optional<double> slope = 10.0;
		std::optional<double> area = 400.0;
		std::optional<double> cell;
		std::optional<bool> strict;
		std::optional<bool> overwrite;
		std::optional<int> epsg;

		std::vector<std::string> tile_paths;
		std::string json_path;

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-l", "--length",    "<metres>",  "minimum length for void triangles",      length);
		args.option("-w", "--width",     "<metres>",  "minimum span width for waterbodies",     width);
		args.option("-z", "--height",    "<metres>",  "maximum average height difference",      height);
		args.option("-s", "--slope",     "<degrees>", "maximum slope for water features",       slope);
		args.option("-a", "--area",      "<metres²>", " minimum island and waterbody area",     area);
		args.option("-c", "--cell",      "<metres>",  "cell size for thinning",                 cell);
		args.option("-t", "--strict",                 "disqualify voids with no ground points", strict);
		args.option("-e", "--epsg",      "<code>",    "EPSG code to set in output file",        epsg);
		args.option("-o", "--overwrite",              "overwrite existing output file",         overwrite);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.ply>",   "input PLY path",      tile_paths);
		args.position("<polys.json>", "output GeoJSON path", json_path);

		if (!args.parse())
			return EXIT_SUCCESS;

		if (!cell)
			cell = length.value() / std::sqrt(8.0);
		if (!width)
			width = 0.0;
		if (length.value() < 0)
			throw std::runtime_error("void length can't be negative");
		if (width.value() < 0)
			throw std::runtime_error("span width can't be negative");
		if (height.value() < 0)
			throw std::runtime_error("average height difference can't be negative");
		if (slope.value() < 0)
			throw std::runtime_error("slope can't be negative");
		if (area.value() < 0)
			throw std::runtime_error("minimum area can't be negative");
		if (cell.value() < 0)
			throw std::runtime_error("cell size can't be negative");
		if (!overwrite && json_path != "-" && std::filesystem::exists(json_path))
			throw std::runtime_error("output file already exists");
		if (epsg && std::clamp(epsg.value(), 1024, 32767) != epsg.value())
			throw std::runtime_error("invalid EPSG code");

		auto points = std::accumulate(tile_paths.begin(), tile_paths.end(), Thinned(cell.value()), [&](auto &thinned, const auto &tile_path) {
			return thinned += PLY(tile_path);
		})();
		auto mesh = Triangulate(points)();
		auto polygons = Polygon::from_mesh(mesh, length.value(), width.value(), height.value(), slope.value(), area.value(), cell.value(), (bool)strict);

		std::stringstream json;
		json.precision(10);

		json << "{\"type\":\"FeatureCollection\",";
		if (epsg)
			json << "\"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"urn:ogc:def:crs:EPSG::" << epsg.value() << "\"}},";
		json << "\"features\":";
		bool first = true;
		for (const auto &polygon: polygons)
			json << (std::exchange(first, false) ? '[' : ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		json << (first ? "[]}" : "]}") << std::endl;

		if (json_path == "-")
			std::cout << json.str();
		else {
			std::ofstream file(json_path);
			file.exceptions(file.exceptions() | std::ofstream::failbit | std::ofstream::badbit);
			file << json.str();
		}
		return EXIT_SUCCESS;
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem reading or writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
