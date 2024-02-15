////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include "args.hpp"
#include "srs.hpp"
#include "output.hpp"
#include "log.hpp"
#include "points.hpp"
#include "mesh.hpp"
#include "polygons.hpp"
#include <algorithm>
#include <thread>
#include <optional>
#include <vector>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
	auto static constexpr pi = 3.14159265358979324;
	auto static const default_threads = std::max<int>(1, std::thread::hardware_concurrency());

	try {
		auto width      = std::optional<double>();
		auto area       = std::optional<double>();
		auto delta      = std::optional<double>(1.5);
		auto slope      = std::optional<double>(5.0);
		auto water      = std::optional<bool>();
		auto simplify   = std::optional<bool>();
		auto smooth     = std::optional<bool>();
		auto angle      = std::optional<double>();
		auto discard    = std::optional<std::vector<int>>{{0,1,7,9,12,18}};
		auto convention = std::optional<std::string>();
		auto multi      = std::optional<bool>();
		auto epsg       = std::optional<int>();
		auto threads    = std::optional<std::vector<int>>{{default_threads}};
		auto tiles_path = std::optional<std::filesystem::path>();
		auto overwrite  = std::optional<bool>();
		auto quiet      = std::optional<bool>();

		auto tile_paths = std::vector<std::filesystem::path>();
		auto output_path = std::filesystem::path();

		Args args(argc, argv, "extract land areas from lidar tiles");
		args.option("-w", "--width",      "<metres>",    "minimum waterbody width",                   width);
		args.option("",   "--area",       "<metres²>",   " minimum waterbody and island area",        area);
		args.option("",   "--delta",      "<metres>",    "maximum waterbody height delta",            delta);
		args.option("",   "--slope",      "<degrees>",   "maximum waterbody slope",                   slope);
		args.option("",   "--water",                     "extract waterbodies instead of land areas", water);
		args.option("",   "--simplify",                  "simplify output polygons",                  simplify);
		args.option("",   "--smooth",                    "smooth output polygons",                    smooth);
		args.option("",   "--angle",      "<degrees>",   "smooth output with given angle",            angle);
		args.option("",   "--discard",    "<class,...>", "discard point classes",                     discard);
		args.option("",   "--convention", "<ogc|esri>",  "force polygon convention to OGC or ESRI",   convention);
		args.option("",   "--multi",                     "collect polygons into single multipolygon", multi);
		args.option("",   "--epsg",       "<number>",    "override missing or incorrect EPSG codes",  epsg);
		args.option("",   "--threads",    "<number>",    "number of processing threads",              threads);
		args.option("",   "--tiles",      "<tiles.txt>", "list of input tiles as a text file",        tiles_path);
		args.option("-o", "--overwrite",                 "overwrite existing output file",            overwrite);
		args.option("-q", "--quiet",                     "don't show progress information",           quiet);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.las>", "LAS input path", tile_paths);
		args.position("<land.json>", "GeoJSON or shapefile output path", output_path);

		auto const proceed = args.parse([&]() {
			if (!width)
				throw std::runtime_error("no width specified");
			if (convention && *convention != "esri" && *convention != "ogc")
				throw std::runtime_error("polygon convention must be 'ogc' or 'esri'");
			if (tiles_path) {
				if (!tile_paths.empty())
					throw std::runtime_error("can't specify tiles as arguments and also in a file");
				if (*tiles_path == "-") {
					std::cin.exceptions(std::ifstream::badbit);
					for (std::string line; std::getline(std::cin, line); )
						tile_paths.emplace_back(line);
				} else {
					auto input = std::ifstream(*tiles_path);
					input.exceptions(std::ifstream::badbit);
					for (std::string line; std::getline(input, line); )
						tile_paths.emplace_back(line);
				}
			}
			if (tile_paths.empty())
				throw std::runtime_error("missing argument: LAS input path");
		});

		if (!proceed)
			return EXIT_SUCCESS;

		if (*width <= 0)
			throw std::runtime_error("width must be positive");
		if (area && *area < 0)
			throw std::runtime_error("area can't be negative");
		if (*delta <= 0)
			throw std::runtime_error("delta must be positive");
		if (*slope <= 0)
			throw std::runtime_error("slope must be positive");
		if (*slope >= 90)
			throw std::runtime_error("slope must be less than 90");
		if (angle && *angle <= 0)
			throw std::runtime_error("smoothing angle must be positive");
		if (angle && *angle >= 180)
			throw std::runtime_error("smoothing angle must be less than 180");
		for (auto klass: *discard)
			if (klass < 0 || klass > 255)
				throw std::runtime_error("invalid lidar point class " + std::to_string(klass));
		if (threads->size() > 2)
			throw std::runtime_error("at most two thread count values allowed");
		for (auto count: *threads)
			if (count < 1)
				throw std::runtime_error("number of threads must be positive");

		if (std::count(tile_paths.begin(), tile_paths.end(), "-") > 1)
			throw std::runtime_error("can't read standard input more than once");

		if (!area)
			area = 4 * *width * *width;
		if (angle)
			smooth = true;
		if (!angle)
			angle = 15.0;

		auto srs = OptionalSRS();
		if (epsg)
			srs.emplace(*epsg);

		auto output = Output(output_path);
		if (!overwrite && output)
			throw std::runtime_error("output file already exists");

		auto const ogc = convention ? *convention == "ogc" : output.ogc();
		auto log = Log(!quiet);

		auto points = Points(tile_paths, *width / std::sqrt(8.0), *discard, water == true, srs, threads->back(), log);
		auto mesh = Mesh(points, threads->front(), log);
		auto polygons = Polygons(mesh, *width, *delta, *slope * pi / 180, water == true, ogc, threads->front(), log);

		if (simplify || smooth) {
			log(Log::Time(), smooth ? "smoothing" : "simplifying", Log::Count(), polygons.ring_count(), "ring");
			auto const tolerance = 4 * *width * *width;
			polygons.simplify(tolerance, water ? ogc : !ogc, threads->front());
		}

		if (smooth) {
			auto const radians = *angle * pi / 180;
			auto const tolerance = 0.5 * *width / std::sin(radians);
			polygons.smooth(tolerance, radians, threads->front());
		}

		if (*area > 0)
			polygons.filter(*area, ogc);

		log(Log::Time(), "saving", Log::Count(), polygons.size(), "polygon");
		if (multi)
			output(polygons.multi(), points.srs());
		else
			output(polygons, points.srs());

		std::exit(EXIT_SUCCESS);
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem reading or writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
