#include "args.hpp"
#include "thinned.hpp"
#include "ply.hpp"
#include "triangulate.hpp"
#include "polygon.hpp"
#include <optional>
#include <thread>
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
		std::optional<std::vector<int>> extra;
		std::optional<int> epsg;
		std::optional<int> jobs = std::max(1u, std::thread::hardware_concurrency());
		std::optional<bool> strict;
		std::optional<bool> overwrite;

		std::vector<std::string> tile_paths;
		std::string json_path;

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-l", "--length",    "<metres>",    "minimum length for void triangles",      length);
		args.option("-w", "--width",     "<metres>",    "minimum span width for waterbodies",     width);
		args.option("-z", "--height",    "<metres>",    "maximum average height difference",      height);
		args.option("-s", "--slope",     "<degrees>",   "maximum slope for water features",       slope);
		args.option("-a", "--area",      "<metres²>",   " minimum island and waterbody area",     area);
		args.option("-c", "--cell",      "<metres>",    "cell resolution for point thinning",     cell);
		args.option("-x", "--extra",     "<class,...>", "extra lidar point classes to consider",  extra);
		args.option("-e", "--epsg",      "<number>",    "EPSG code to set in output file",        epsg);
		args.option("-j", "--jobs",      "<number>",    "number of threads when processing",      jobs);
		args.option("-t", "--strict",                   "disqualify voids with no ground points", strict);
		args.option("-o", "--overwrite",                "overwrite existing output file",         overwrite);
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
			width = length.value();
		if (!extra)
			extra.emplace();

		if (length.value() <= 0.0)
			throw std::runtime_error("void length must be positive");
		if (width.value() <= 0.0)
			throw std::runtime_error("span width must be positive");
		if (height.value() <= 0.0)
			throw std::runtime_error("average height difference must be positive");
		if (slope.value() <= 0.0)
			throw std::runtime_error("slope must be positive");
		if (area.value() <= 0.0)
			throw std::runtime_error("minimum area must be positive");
		if (cell.value() <= 0.0)
			throw std::runtime_error("cell size must be positive");
		for (auto klass: extra.value())
			if (std::clamp(klass, 0, 255) != klass)
				throw std::runtime_error("invalid lidar point class");
		if (epsg && std::clamp(epsg.value(), 1024, 32767) != epsg.value())
			throw std::runtime_error("invalid EPSG code");
		if (jobs.value() < 1)
			throw std::runtime_error("number of threads must be positive");
		if (!overwrite && json_path != "-" && std::filesystem::exists(json_path))
			throw std::runtime_error("output file already exists");

		auto points = std::accumulate(tile_paths.begin(), tile_paths.end(), Thinned(cell.value(), extra.value()), [&](auto &thinned, const auto &tile_path) {
			return thinned += PLY(tile_path);
		})();
		auto mesh = Triangulate(points, jobs.value())();
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
