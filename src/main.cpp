#include "args.hpp"
#include "mesh.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>

int main(int argc, char *argv[]) {
	try {
		double noise = 1.0;
		double length = 10.0;
		double slope = 0.5;
		double area = 100.0;
		int consensus = 10;
		int iterations = 100;
		std::string ply_path;
		std::string json_path;

		Args args(argc, argv, "extract land areas from triangulated lidar tiles");
		args.option("-n", "--noise",      "<metres>",  "RANSAC noise threshold",              noise);
		args.option("-l", "--length",     "<metres>",  "minimum triangle length for voids",   length);
		args.option("-s", "--slope",      "<degrees>", "maximum slope angle for water voids", slope);
		args.option("-a", "--area",       "<m²>",      " minimum area for islands and ponds", area);
		args.option("-c", "--consensus",  "<count>",   "number of consensus points required", consensus);
		args.option("-i", "--iterations", "<count>",   "number of RANSAC iterations",         iterations);
		args.position("<tin.ply>",    "input PLY path",      ply_path);
		args.position("<polys.json>", "output GeoJSON path", json_path);

		if (!args.parse())
			return EXIT_SUCCESS;

		if (noise < 0)
			throw std::runtime_error("noise threshold can't be negative");
		if (length < 0)
			throw std::runtime_error("length can't be negative");
		if (slope < 0)
			throw std::runtime_error("slope can't be negative");
		if (area < 0)
			throw std::runtime_error("minimum area can't be negative");
		if (consensus < 3)
			throw std::runtime_error("consensus must be at least 3");
		if (iterations <= 0)
			throw std::runtime_error("iterations must be positive");

		Mesh mesh(ply_path);
		mesh.remove_voids(noise, length, slope, consensus, iterations);
		auto polygons = mesh.polygons(area);

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
