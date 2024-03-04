////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef APP_HPP
#define APP_HPP

#include "args.hpp"
#include "srs.hpp"
#include "log.hpp"
#include "vector.hpp"
#include "summation.hpp"
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <optional>
#include <string>
#include <stdexcept>
#include <fstream>
#include <cstdlib>
#include <cmath>

struct App {
	using Discard = std::unordered_set<unsigned char>;
	using Ints = std::vector<int>;
	using Path = std::filesystem::path;
	using Paths = std::vector<Path>;
	using OptionalPath = std::optional<Path>;

	double       width;
	double       resolution;
	double       area;
	double       delta;
	double       slope;
	bool         land;
	bool         simplify;
	bool         smooth;
	double       angle;
	Discard      discard;
	bool         multi;
	bool         overwrite;
	Paths        tile_paths;
	OptionalPath path;
	OptionalSRS  srs;
	bool         ogc;
	int          threads;
	int          io_threads;
	Log          log;

	auto static parse(int argc, char *argv[]) {
		auto static constexpr pi = 3.14159265358979324;
		auto static const default_threads = std::max<int>(1, std::thread::hardware_concurrency());

		auto width       = std::optional<double>();
		auto area        = std::optional<double>();
		auto delta       = std::optional<double>(1.5);
		auto slope       = std::optional<double>(5.0);
		auto land        = std::optional<bool>();
		auto simplify    = std::optional<bool>();
		auto smooth      = std::optional<bool>();
		auto angle       = std::optional<double>();
		auto discard     = std::optional<Ints>{{0,1,7,9,12,18}};
		auto convention  = std::optional<std::string>();
		auto multi       = std::optional<bool>();
		auto epsg        = std::optional<int>();
		auto threads     = std::optional<Ints>{{default_threads}};
		auto tiles_path  = std::optional<Path>();
		auto overwrite   = std::optional<bool>();
		auto quiet       = std::optional<bool>();

		auto tile_paths  = Paths();
		auto path        = Path();

		auto args = Args(argc, argv, "extract waterbodies from lidar tiles");
		args.option("-w", "--width",      "<metres>",    "minimum waterbody width",                   width);
		args.option("",   "--area",       "<metres²>",   "minimum waterbody and island area",         area);
		args.option("",   "--delta",      "<metres>",    "maximum waterbody height delta",            delta);
		args.option("",   "--slope",      "<degrees>",   "maximum waterbody slope",                   slope);
		args.option("",   "--land",                      "extract land areas instead of waterbodies", land);
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
		args.position("<water.json>", "GeoJSON or shapefile output path", path);

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
			std::exit(EXIT_SUCCESS);

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

		if (!convention)
			convention = path.extension() == ".shp" ? "esri" : "ogc";

		auto app = App {
			.log         = Log(!quiet),
			.width       = *width,
			.resolution  = *width / std::sqrt(8.0),
			.area        = *area,
			.delta       = *delta,
			.slope       = *slope * pi / 180,
			.land        = land.has_value(),
			.simplify    = simplify.has_value(),
			.smooth      = smooth.has_value(),
			.angle       = *angle * pi / 180,
			.discard     = Discard(discard->begin(), discard->end()),
			.multi       = multi.has_value(),
			.overwrite   = overwrite.has_value(),
			.tile_paths  = tile_paths,
			.ogc         = *convention == "ogc",
			.threads     = threads->front(),
			.io_threads  = threads->back(),
		};

		if (path == "-")
			;
		else if (path.extension() == ".json")
			app.path = path;
		else if (path.extension() == ".shp")
			app.path = path;
		else
			throw std::runtime_error("output file extension must be .json or .shp");

		if (epsg)
			app.srs.emplace(*epsg);

		return app;
	}

	template <typename Triangles>
	auto is_water(Triangles const &triangles) const {
		auto perp_sum = Vector<3>{{0.0, 0.0, 0.0}};
		auto perp_sum_z = Summation(perp_sum[2]);

		auto delta_sum = 0.0;
		auto delta_count = 0ul;
		auto delta_summer = Summation(delta_sum);

		for (auto edges: triangles) {
			std::rotate(edges.begin(), std::min_element(edges.begin(), edges.end(), [](auto const &edge1, auto const &edge2) {
				return (*edge1.second - *edge1.first).sqnorm() < (*edge2.second - *edge2.first).sqnorm();
			}), edges.end());

			auto const perp = edges[1] ^ edges[2];
			auto const &p0 = *edges[0].first;
			auto const &p1 = *edges[1].first;
			auto const &p2 = *edges[2].first;

			if (p0.withheld || p1.withheld || p2.withheld) {
				perp_sum_z += perp.norm();
				delta_count += 2;
			} else if (p0.ground() && p1.ground() && p2.ground()) {
				perp_sum[0] += perp[0];
				perp_sum[1] += perp[1];
				perp_sum_z  += perp[2];
				delta_summer += std::abs(p1.elevation - p2.elevation);
				delta_summer += std::abs(p2.elevation - p0.elevation);
				delta_count += 2;
			}
		}

		return delta_sum < delta * delta_count && std::abs(perp_sum[2]) > std::cos(slope) * perp_sum.norm();
	}
};

#endif
