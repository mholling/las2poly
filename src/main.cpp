#include "args.hpp"
#include "thinned.hpp"
#include "ply.hpp"
#include "triangulate.hpp"
#include "polygon.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <utility>
#include <iostream>

int main(int argc, char *argv[]) {
	try {
		double length = 10.0;
		double width = 0.0;
		double height = 5.0;
		double slope = 10.0;
		double area = 400.0;
		double cell = 0.0;
		bool strict = false;
		bool overwrite = false;
		std::string epsg;
		std::vector<std::string> tile_paths;
		std::string json_path;

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-l", "--length",    "<metres>",  "minimum length for void triangles",      length);
		args.option("-w", "--width",     "<metres>",  "minimum span width of water features",   width);
		args.option("-z", "--height",    "<metres>",  "maximum average height difference",      height);
		args.option("-s", "--slope",     "<degrees>", "maximum slope for water features",       slope);
		args.option("-a", "--area",      "<metres²>", " minimum area for islands and ponds",    area);
		args.option("-c", "--cell",      "<metres>",  "cell size for thinning, 0 for auto",     cell);
		args.option("-t", "--strict",                 "disqualify voids with no ground points", strict);
		args.option("-o", "--overwrite",              "overwrite existing output file",         overwrite);
		args.option("-e", "--epsg",      "<code>",    "set EPSG code in output file",           epsg);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.ply>",   "input PLY path",      tile_paths);
		args.position("<polys.json>", "output GeoJSON path", json_path);

		if (!args.parse())
			return EXIT_SUCCESS;

		if (length < 0)
			throw std::runtime_error("void length can't be negative");
		if (width < 0)
			throw std::runtime_error("span width can't be negative");
		if (height < 0)
			throw std::runtime_error("average height difference can't be negative");
		if (slope < 0)
			throw std::runtime_error("slope can't be negative");
		if (area < 0)
			throw std::runtime_error("minimum area can't be negative");
		if (cell < 0)
			throw std::runtime_error("cell size can't be negative");
		if (cell == 0)
			cell = length / std::sqrt(8.0);

		if (!overwrite && std::filesystem::exists(json_path))
			throw std::runtime_error("output file already exists");

		if (!epsg.empty()) {
			auto valid = std::all_of(epsg.begin(), epsg.end(), [](const auto character) {
				return std::clamp(character, '0', '9') == character;
			}) && std::clamp(epsg.size(), 4ul, 5ul) == epsg.size();
			if (!valid)
				throw std::runtime_error("invalid EPSG code");
		}

		auto points = std::accumulate(tile_paths.begin(), tile_paths.end(), Thinned(cell), [&](auto &thinned, const auto &tile_path) {
			return thinned += PLY(tile_path);
		}).to_vector();
		auto mesh = Triangulate(points)();
		auto polygons = Polygon::from_mesh(mesh, length, width, height, slope, area, cell, strict);

		std::ofstream json;
		json.exceptions(json.exceptions() | std::ofstream::failbit);
		json.open(json_path);
		json.precision(10);

		json << "{\"type\":\"FeatureCollection\",";
		if (!epsg.empty())
			json << "\"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"urn:ogc:def:crs:EPSG::" << epsg << "\"}},";
		json << "\"features\":";
		bool first = true;
		for (const auto &polygon: polygons)
			json << (std::exchange(first, false) ? '[' : ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		json << (first ? "[]}" : "]}");
		return EXIT_SUCCESS;
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem reading or writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
