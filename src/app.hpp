////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef APP_HPP
#define APP_HPP

#include "opts.hpp"
#include "srs.hpp"
#include "log.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <unordered_set>
#include <filesystem>
#include <vector>
#include <optional>

class App {
	App(Opts const &&opts) :
		width      (opts.width),
		delta      (*opts.delta),
		min_cosine (std::cos(*opts.slope * std::numbers::pi / 180)),
		land       (opts.land),
		area       (opts.area),
		scale      (opts.scale),
		simplify   (!opts.raw),
		smooth     (!opts.raw && !opts.simplify),
		multi      (opts.multi),
		lines      (opts.lines),
		discard    (opts.discard->begin(), opts.discard->end()),
		overwrite  (opts.overwrite),
		tile_paths (opts.tile_paths),
		threads    (opts.threads->front()),
		io_threads (opts.threads->back()),
		log        (!opts.quiet)
	{
		if (opts.path == "-")
			;
		else if (opts.path.extension() == ".json")
			path = opts.path;
		else if (opts.path.extension() == ".shp")
			path = opts.path;
		else
			throw std::runtime_error("output file extension must be .json or .shp");

		if (opts.epsg)
			srs.emplace(*opts.epsg);
	}

	using Discard = std::unordered_set<unsigned char>;
	using Path = std::filesystem::path;
	using Paths = std::vector<Path>;
	using OptionalPath = std::optional<Path>;
	using OptionalDouble = std::optional<double>;

public:
	OptionalDouble width;
	double         delta;
	double         min_cosine;
	bool           land;
	OptionalDouble area;
	OptionalDouble scale;
	bool           simplify;
	bool           smooth;
	bool           multi;
	bool           lines;
	Discard        discard;
	bool           overwrite;
	Paths          tile_paths;
	OptionalPath   path;
	OptionalSRS    srs;
	int            threads;
	int            io_threads;
	Log            log;

	App(int argc, char *argv[]) :
		App(Opts(argc, argv))
	{ }
};

#endif
