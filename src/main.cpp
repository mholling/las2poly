#include "args.hpp"
#include "polygon.hpp"
#include <string>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <utility>

int main(int argc, char *argv[]) {
	try {
		double length = 10.0;
		double width = 0.0;
		double height = 5.0;
		double slope = 10.0;
		double area = 400.0;
		double cell = 0.0;
		bool strict = false;
		std::string ply_path;
		std::string json_path;

		Args args(argc, argv, "extract land areas from triangulated lidar tiles");
		args.option("-l", "--length", "<metres>",  "minimum length for void triangles",      length);
		args.option("-w", "--width",  "<metres>",  "minimum span width of water features",   width);
		args.option("-z", "--height", "<metres>",  "maximum average height difference",      height);
		args.option("-s", "--slope",  "<degrees>", "maximum slope for water features",       slope);
		args.option("-a", "--area",   "<metres²>", " minimum area for islands and ponds",    area);
		args.option("-c", "--cell",   "<metres>",  "cell size for thinning, 0 for auto",     cell);
		args.option("-t", "--strict",              "disqualify voids with no ground points", strict);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tin.ply>",    "input PLY path",      ply_path);
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

		auto polygons = Polygon::from_ply(ply_path, length, width, height, slope, area, cell, strict);

		std::ofstream json;
		json.exceptions(json.exceptions() | std::ofstream::failbit);
		json.open(json_path);
		json.precision(10);

		json << "{\"type\":\"FeatureCollection\",\"features\":";
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
