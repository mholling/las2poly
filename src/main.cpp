#include "args.hpp"
#include "logger.hpp"
#include "points.hpp"
#include "mesh.hpp"
#include "land.hpp"
#include <optional>
#include <vector>
#include <algorithm>
#include <thread>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdlib>

int main(int argc, char *argv[]) {
	static constexpr auto pi = 3.14159265358979323846264338327950288419716939937510;
	static constexpr auto max_web_mercator = 20048966.10;
	static constexpr auto min_resolution = max_web_mercator / INT32_MAX;

	try {
		std::optional<double> width;
		std::optional<double> slope = 10;
		std::optional<double> area;
		std::optional<double> length;
		std::optional<double> resolution;
		std::optional<double> simplify;
		std::optional<double> smooth;
		std::optional<bool> automatic;
		std::optional<double> angle = 15;
		std::optional<std::vector<int>> classes;
		std::optional<int> epsg;
		std::optional<int> threads = std::max(1u, std::thread::hardware_concurrency());
		std::optional<bool> overwrite;
		std::optional<bool> progress;

		std::vector<std::string> tile_paths;
		std::string json_path;

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-w", "--width",      "<metres>",    "minimum waterbody width",                width);
		args.option("-s", "--slope",      "<degrees>",   "maximum waterbody slope",                slope);
		args.option("-a", "--area",       "<metres²>",   " minimum waterbody and island area",     area);
		args.option("-l", "--length",     "<metres>",    "minimum edge length for void triangles", length);
		args.option("-r", "--resolution", "<metres>",    "point thinning resolution",              resolution);
		args.option("-i", "--simplify",   "<metres²>",   " output simplification tolerance",       simplify);
		args.option("-m", "--smooth",     "<metres>",    "output smoothing tolerance",             smooth);
		args.option("-u", "--auto",                      "automatic simplification and smoothing", automatic);
		args.option("-g", "--angle",      "<degrees>",   "threshold smoothing angle",              angle);
		args.option("-c", "--classes",    "<class,...>", "additional lidar point classes",         classes);
		args.option("-e", "--epsg",       "<number>",    "EPSG code to set in output file",        epsg);
		args.option("-t", "--threads",    "<number>",    "number of processing threads",           threads);
		args.option("-o", "--overwrite",                 "overwrite existing output file",         overwrite);
		args.option("-p", "--progress",                  "show progress",                          progress);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.las>", "LAS or PLY input path", tile_paths);
		args.position("<land.json>", "GeoJSON output path", json_path);

		auto proceed = args.parse([&]() {
			if (!length && !width)
				throw std::runtime_error("no width or length specified");
		});

		if (!proceed)
			return EXIT_SUCCESS;

		if (!classes)
			classes.emplace();

		if (width && width.value() <= 0)
			throw std::runtime_error("width must be positive");
		if (slope.value() <= 0)
			throw std::runtime_error("slope must be positive");
		if (slope.value() >= 90)
			throw std::runtime_error("slope must be less than 90");
		if (length && length.value() <= 0)
			throw std::runtime_error("edge length must be positive");
		if (length && width && length.value() > width.value())
			throw std::runtime_error("edge length can't be more than width");
		if (area && area.value() < 0)
			throw std::runtime_error("area can't be negative");
		if (resolution && resolution.value() < min_resolution)
			throw std::runtime_error("resolution value too low");
		if (simplify && simplify.value() < 0)
			throw std::runtime_error("simplification tolerance can't be negative");
		if (smooth && smooth.value() < 0)
			throw std::runtime_error("smoothing tolerance can't be negative");
		if (angle.value() <= 0)
			throw std::runtime_error("smoothing angle must be positive");
		if (angle.value() >= 180)
			throw std::runtime_error("smoothing angle must be less than 180");
		for (auto klass: classes.value()) {
			if (klass < 0 || klass > 255)
				throw std::runtime_error("invalid lidar point class " + std::to_string(klass));
			if (7 == klass || 9 == klass || 12 == klass || 18 == klass)
				throw std::runtime_error("can't use lidar point class " + std::to_string(klass));
		}
		if (epsg && (epsg.value() < 1024 || epsg.value() > 32767))
			throw std::runtime_error("invalid EPSG code");
		if (threads.value() < 1)
			throw std::runtime_error("number of threads must be positive");
		if (!overwrite && json_path != "-" && std::filesystem::exists(json_path))
			throw std::runtime_error("output file already exists");

		if (std::count(tile_paths.begin(), tile_paths.end(), "-") > 1)
			throw std::runtime_error("can't read standard input more than once");

		if (!width)
			width = length.value();
		if (!length)
			length = width.value();
		if (!area)
			area = 4 * width.value() * width.value();
		if (!resolution)
			resolution = length.value() / std::sqrt(8.0);
		if (automatic && !simplify)
			simplify = 4 * width.value() * width.value();
		if (automatic && !smooth)
			smooth = 0.25 * std::sqrt(simplify.value()) / std::sin(angle.value() * pi / 180);

		auto logger = Logger((bool)progress);

		if (automatic) {
			logger.info("simplification tolerance: ", simplify.value(), "m²");
			logger.info("smoothing tolerance: ", smooth.value(), "m");
			logger.info("smoothing angle: ", angle.value(), "°");
		}

		logger.time("reading", tile_paths.size(), "file");
		auto points = Points(tile_paths, resolution.value(), classes.value(), threads.value());

		logger.time("triangulating", points.size(), "point");
		auto mesh = Mesh(points, threads.value());

		logger.time("extracting polygons");
		auto land = Land(mesh, length.value(), width.value(), slope.value() * pi / 180, area.value(), threads.value());

		if (simplify && simplify.value() > 0)
			land.simplify(simplify.value());
		if (smooth && smooth.value() > 0)
			land.smooth(smooth.value(), angle.value() * pi / 180);

		std::stringstream json;
		json.precision(15);
		json << "{\"type\":\"FeatureCollection\",";
		if (epsg)
			json << "\"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"urn:ogc:def:crs:EPSG::" << epsg.value() << "\"}},";
		json << "\"features\":" << land << "}";

		logger.time("saving", land.size(), "polygon");
		if (json_path == "-")
			std::cout << json.str() << std::endl;
		else {
			std::ofstream file(json_path);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file << json.str() << std::endl;
		}
		std::exit(EXIT_SUCCESS);
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
