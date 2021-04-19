////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include "args.hpp"
#include "logger.hpp"
#include "points.hpp"
#include "mesh.hpp"
#include "polygons.hpp"
#include <optional>
#include <vector>
#include <algorithm>
#include <thread>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[]) {
	auto static constexpr pi = 3.14159265358979324;
	auto static constexpr smoothing_angle = 15.0;
	auto static const default_threads = std::max<int>(1, std::thread::hardware_concurrency());

	try {
		auto width      = std::optional<double>();
		auto slope      = std::optional<double>(10.0);
		auto area       = std::optional<double>();
		auto length     = std::optional<double>();
		auto simplify   = std::optional<bool>();
		auto smooth     = std::optional<bool>();
		auto discard    = std::optional<std::vector<int>>{{0,1,7,9,12,18}};
		auto epsg       = std::optional<int>();
		auto threads    = std::optional<std::vector<int>>{{default_threads}};
		auto tiles_path = std::optional<std::string>();
		auto overwrite  = std::optional<bool>();
		auto progress   = std::optional<bool>();

		auto tile_paths = std::vector<std::string>();
		auto json_path = std::string();

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-w", "--width",      "<metres>",    "minimum waterbody width",                width);
		args.option("-s", "--slope",      "<degrees>",   "maximum waterbody slope",                slope);
		args.option("-a", "--area",       "<metres²>",   " minimum waterbody and island area",     area);
		args.option("-l", "--length",     "<metres>",    "minimum edge length for void triangles", length);
		args.option("-i", "--simplify",                  "apply output simplification",            simplify);
		args.option("-m", "--smooth",                    "apply output smoothing",                 smooth);
		args.option("-d", "--discard",    "<class,...>", "discarded point classes",                discard);
		args.option("-e", "--epsg",       "<number>",    "EPSG code to set in output file",        epsg);
		args.option("-t", "--threads",    "<number>",    "number of processing threads",           threads);
		args.option("-x", "--tiles",      "<tiles.txt>", "list of input tiles as a text file",     tiles_path);
		args.option("-o", "--overwrite",                 "overwrite existing output file",         overwrite);
		args.option("-p", "--progress",                  "show progress",                          progress);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.las>", "LAS input path", tile_paths);
		args.position("<land.json>", "GeoJSON output path", json_path);

		auto const proceed = args.parse([&]() {
			if (!length && !width)
				throw std::runtime_error("no width or length specified");
			if (tiles_path && !tile_paths.empty())
				throw std::runtime_error("can't specify tiles as arguments and also in a file");
			if (tiles_path) {
				auto input = std::ifstream(*tiles_path);
				input.exceptions(std::ifstream::badbit);
				for (std::string line; std::getline(input, line); )
					tile_paths.push_back(line);
			}
			if (tile_paths.empty())
				throw std::runtime_error("missing argument: LAS input path");
		});

		if (!proceed)
			return EXIT_SUCCESS;

		if (width && *width <= 0)
			throw std::runtime_error("width must be positive");
		if (*slope <= 0)
			throw std::runtime_error("slope must be positive");
		if (*slope >= 90)
			throw std::runtime_error("slope must be less than 90");
		if (length && *length <= 0)
			throw std::runtime_error("edge length must be positive");
		if (length && width && *length > *width)
			throw std::runtime_error("edge length can't be more than width");
		if (area && *area < 0)
			throw std::runtime_error("area can't be negative");
		for (auto klass: *discard)
			if (klass < 0 || klass > 255)
				throw std::runtime_error("invalid lidar point class " + std::to_string(klass));
		if (epsg && (*epsg < 1024 || *epsg > 32767))
			throw std::runtime_error("invalid EPSG code");
		if (threads->size() > 2)
			throw std::runtime_error("at most two thread count values allowed");
		for (auto count: *threads)
			if (count < 1)
				throw std::runtime_error("number of threads must be positive");
		if (!overwrite && json_path != "-" && std::filesystem::exists(json_path))
			throw std::runtime_error("output file already exists");

		if (std::count(tile_paths.begin(), tile_paths.end(), "-") > 1)
			throw std::runtime_error("can't read standard input more than once");

		if (!width)
			width = *length;
		if (!length)
			length = *width;
		if (!area)
			area = 4 * *width * *width;
		if (smooth)
			simplify = true;

		auto logger = Logger((bool)progress);

		logger.time("reading", tile_paths.size(), "file");
		auto points = Points(tile_paths, *length / std::sqrt(8.0), *discard, threads->back());

		logger.time("triangulating", points.size(), "point");
		auto mesh = Mesh(points, threads->front());

		logger.time("extracting polygons");
		auto polygons = Polygons(mesh, *length, *width, *slope * pi / 180, *area, threads->front());

		if (simplify) {
			auto const tolerance = 4 * *width * *width;
			polygons.simplify(tolerance);
		}

		if (smooth) {
			auto const angle = smoothing_angle * pi / 180;
			auto const tolerance = 0.5 * *width / std::sin(angle);
			polygons.smooth(tolerance, angle);
		}

		auto json = std::stringstream();
		json.precision(15);
		json << "{\"type\":\"FeatureCollection\",";
		if (epsg)
			json << "\"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"urn:ogc:def:crs:EPSG::" << *epsg << "\"}},";
		json << "\"features\":" << polygons << "}";

		logger.time("saving", polygons.size(), "polygon");
		if (json_path == "-")
			std::cout << json.str() << std::endl;
		else {
			auto file = std::ofstream(json_path);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file << json.str() << std::endl;
		}
		std::exit(EXIT_SUCCESS);
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem reading or writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
