#include "args.hpp"
#include "polygon.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>

int main(int argc, char *argv[]) {
	try {
		double length = 10.0;
		double width = 0.0;
		double height = 5.0;
		double slope = 10.0;
		double area = 400.0;
		std::string ply_path;
		std::string json_path;

		Args args(argc, argv, "extract land areas from triangulated lidar tiles");
		args.option("-l", "--length",     "<metres>",  "minimum length for void triangles",    length);
		args.option("-w", "--width",      "<metres>",  "minimum span width of water features", width);
		args.option("-z", "--height",     "<metres>",  "maximum RMS height difference",        height);
		args.option("-s", "--slope",      "<degrees>", "maximum slope for water features",     slope);
		args.option("-a", "--area",       "<metres²>", " minimum area for islands and ponds",  area);
		args.position("<tin.ply>",    "input PLY path",      ply_path);
		args.position("<polys.json>", "output GeoJSON path", json_path);

		if (!args.parse())
			return EXIT_SUCCESS;

		if (length < 0)
			throw std::runtime_error("void length can't be negative");
		if (width < 0)
			throw std::runtime_error("span width can't be negative");
		if (height < 0)
			throw std::runtime_error("RMS height can't be negative");
		if (slope < 0)
			throw std::runtime_error("slope can't be negative");
		if (area < 0)
			throw std::runtime_error("minimum area can't be negative");

		auto polygons = Polygon::from_ply(ply_path, length, width, height, slope, area);

		std::ofstream json;
		json.exceptions(json.exceptions() | std::ofstream::failbit);
		json.open(json_path);
		json.precision(10);

		json << "{\"type\":\"FeatureCollection\",\"features\":";
		unsigned int count = 0;
		for (const auto &polygon: polygons)
			json << (count++ ? ',' : '[') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		json << (count ? "]}" : "[]}");
		return EXIT_SUCCESS;
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem reading or writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
