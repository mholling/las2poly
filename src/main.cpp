#include "args.hpp"
#include "thin.hpp"
#include "tile.hpp"
#include "triangulate.hpp"
#include "land.hpp"
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
		std::optional<double> width = 10.0;
		std::optional<double> slope = 10.0;
		std::optional<double> delta = 5.0;
		std::optional<double> length;
		std::optional<double> area;
		std::optional<double> resolution;
		std::optional<double> simplify;
		std::optional<double> smooth;
		std::optional<std::vector<int>> classes;
		std::optional<int> epsg;
		std::optional<int> threads = std::max(1u, std::thread::hardware_concurrency());
		std::optional<bool> permissive;
		std::optional<bool> overwrite;

		std::vector<std::string> tile_paths;
		std::string json_path;

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-w", "--width",      "<metres>",    "minimum width for waterbodies",          width);
		args.option("-s", "--slope",      "<degrees>",   "maximum slope for waterbodies",          slope);
		args.option("-d", "--delta",      "<metres>",    "maximum average height difference",      delta);
		args.option("-l", "--length",     "<metres>",    "minimum edge length for void triangles", length);
		args.option("-a", "--area",       "<metres²>",   " minimum waterbody and island area",     area);
		args.option("-r", "--resolution", "<metres>",    "resolution for point thinning",          resolution);
		args.option("-i", "--simplify",   "<metres²>",   " simplify output to given tolerance",    simplify);
		args.option("-m", "--smooth",     "<metres>",    "smooth output to given tolerance",       smooth);
		args.option("-x", "--classes",    "<class,...>", "additional lidar point classes",         classes);
		args.option("-e", "--epsg",       "<number>",    "EPSG code to set in output file",        epsg);
		args.option("-t", "--threads",    "<number>",    "number of processing threads",           threads);
		args.option("-p", "--permissive",                "allow voids with no ground points",      permissive);
		args.option("-o", "--overwrite",                 "overwrite existing output file",         overwrite);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.las>", "LAS or PLY input path", tile_paths);
		args.position("<land.json>", "GeoJSON output path", json_path);

		if (!args.parse())
			return EXIT_SUCCESS;

		if (!length)
			length = width.value();
		if (!area)
			area = 4 * width.value() * width.value();
		if (!resolution)
			resolution = length.value() / std::sqrt(8.0);
		if (!classes)
			classes.emplace();

		if (width.value() <= 0.0)
			throw std::runtime_error("width must be positive");
		if (slope.value() <= 0.0)
			throw std::runtime_error("slope must be positive");
		if (delta.value() <= 0.0)
			throw std::runtime_error("average height difference must be positive");
		if (length.value() <= 0.0)
			throw std::runtime_error("edge length must be positive");
		if (length.value() > width.value())
			throw std::runtime_error("edge length can't be more than width");
		if (area.value() < 0.0)
			throw std::runtime_error("area can't be negative");
		if (resolution.value() <= 0.0)
			throw std::runtime_error("resolution must be positive");
		if (smooth && smooth.value() <= 0.0)
			throw std::runtime_error("smoothing tolerance must be positive");
		for (auto klass: classes.value()) {
			if (std::clamp(klass, 0, 255) != klass)
				throw std::runtime_error("invalid lidar point class " + std::to_string(klass));
			if (7 == klass || 9 == klass || 18 == klass)
				throw std::runtime_error("can't use lidar point class " + std::to_string(klass));
		}
		if (epsg && std::clamp(epsg.value(), 1024, 32767) != epsg.value())
			throw std::runtime_error("invalid EPSG code");
		if (threads.value() < 1)
			throw std::runtime_error("number of threads must be positive");
		if (!overwrite && json_path != "-" && std::filesystem::exists(json_path))
			throw std::runtime_error("output file already exists");

		auto points = std::accumulate(tile_paths.begin(), tile_paths.end(), Thin(resolution.value(), classes.value()), [&](auto &thin, const auto &tile_path) {
			return thin += Tile(tile_path);
		})();
		auto mesh = Triangulate(points, threads.value())();
		auto land = Land(mesh, length.value(), width.value(), delta.value(), slope.value(), area.value(), (bool)permissive);

		if (simplify)
			for (auto &polygon: land)
				for (auto &ring: polygon)
					ring.simplify(simplify.value());

		if (smooth)
			for (auto &polygon: land)
				for (auto &ring: polygon)
					ring.smooth(smooth.value(), 15.0);

		std::stringstream json;
		json.precision(12);
		json << "{\"type\":\"FeatureCollection\",";
		if (epsg)
			json << "\"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"urn:ogc:def:crs:EPSG::" << epsg.value() << "\"}},";
		json << "\"features\":" << land << "}" << std::endl;

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
