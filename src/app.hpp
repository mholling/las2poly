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
		area       (opts.area),
		delta      (*opts.delta),
		slope      (*opts.slope * std::numbers::pi / 180),
		land       (opts.land),
		simplify   (opts.simplify),
		smooth     (!opts.raw && !opts.simplify),
		discard    (opts.discard->begin(), opts.discard->end()),
		multi      (opts.multi),
		lines      (opts.lines),
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
	OptionalDouble area;
	double         delta;
	double         slope;
	bool           land;
	bool           simplify;
	bool           smooth;
	Discard        discard;
	bool           multi;
	bool           lines;
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
