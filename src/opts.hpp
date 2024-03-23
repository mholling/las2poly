////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef OPTS_HPP
#define OPTS_HPP

#include "args.hpp"
#include <vector>
#include <filesystem>
#include <optional>
#include <algorithm>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

struct Opts {
	using Ints = std::vector<int>;
	using Path = std::filesystem::path;
	using Paths = std::vector<Path>;

	std::optional<double> width;
	std::optional<double> delta;
	std::optional<double> slope;
	std::optional<bool>   land;
	std::optional<double> area;
	std::optional<double> scale;
	std::optional<bool>   simplify;
	std::optional<bool>   raw;
	std::optional<Ints>   discard;
	std::optional<bool>   multi;
	std::optional<bool>   lines;
	std::optional<int>    epsg;
	std::optional<Ints>   threads;
	std::optional<Path>   tiles_path;
	std::optional<bool>   overwrite;
	std::optional<bool>   quiet;

	Paths tile_paths;
	Path  path;

	Opts(int argc, char *argv[]) :
		delta(1.5),
		slope(5.0),
		discard{{0,1,7,9,12,18}},
		threads{{std::max<int>(1, std::thread::hardware_concurrency())}}
	{
		auto args = Args(argc, argv, "extract waterbodies from lidar tiles");
		args.option("-w", "--width",      "<metres>",    "minimum waterbody width",                        width);
		args.option("",   "--delta",      "<metres>",    "maximum waterbody height delta",                 delta);
		args.option("",   "--slope",      "<degrees>",   "maximum waterbody slope",                        slope);
		args.option("",   "--land",                      "extract land areas instead of waterbodies",      land);
		args.option("",   "--area",       "<metres²>",   "minimum waterbody and island area",              area);
		args.option("",   "--scale",      "<metres>",    "feature scale for smoothing and simplification", scale);
		args.option("",   "--simplify",                  "simplify output polygons",                       simplify);
		args.option("",   "--raw",                       "don't smooth output polygons",                   raw);
		args.option("",   "--discard",    "<class,...>", "discard point classes",                          discard);
		args.option("",   "--multi",                     "collect polygons into single multipolygon",      multi);
		args.option("",   "--lines",                     "output polygon boundaries as linestrings",       lines);
		args.option("",   "--epsg",       "<number>",    "override missing or incorrect EPSG codes",       epsg);
		args.option("",   "--threads",    "<number>",    "number of processing threads",                   threads);
		args.option("",   "--tiles",      "<tiles.txt>", "list of input tiles as a text file",             tiles_path);
		args.option("-o", "--overwrite",                 "overwrite existing output file",                 overwrite);
		args.option("-q", "--quiet",                     "don't show progress information",                quiet);
#ifdef VERSION
		args.version(VERSION);
#endif
		args.position("<tile.las>", "LAS input path", tile_paths);
		args.position("<water.json>", "GeoJSON or shapefile output path", path);

		auto const proceed = args.parse([&]() {
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
			std::exit(EXIT_SUCCESS);

		if (width && *width <= 0)
			throw std::runtime_error("width must be positive");
		if (area && *area < 0)
			throw std::runtime_error("area can't be negative");
		if (*delta <= 0)
			throw std::runtime_error("delta must be positive");
		if (*slope <= 0)
			throw std::runtime_error("slope must be positive");
		if (*slope >= 90)
			throw std::runtime_error("slope must be less than 90");
		if (scale && *scale < 0)
			throw std::runtime_error("scale can't be negative");
		for (auto klass: *discard)
			if (klass < 0 || klass > 255)
				throw std::runtime_error("invalid lidar point class " + std::to_string(klass));
		if (threads->size() > 2)
			throw std::runtime_error("at most two thread count values allowed");
		for (auto count: *threads)
			if (count < 1)
				throw std::runtime_error("number of threads must be positive");
		if (auto count = std::count(tile_paths.begin(), tile_paths.end(), "-"); count > 1)
			throw std::runtime_error("can't read standard input more than once");
		else if (count > 0 && !width)
			throw std::runtime_error("can't estimate width from standard input");
		if (raw && simplify)
			throw std::runtime_error("either raw or simplify but not both");
	}
};

#endif
