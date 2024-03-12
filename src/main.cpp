////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#include "app.hpp"
#include "output.hpp"
#include "defaults.hpp"
#include "points.hpp"
#include "mesh.hpp"
#include "edges.hpp"
#include "polygons.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[]) {
	try {
		auto app = App(argc, argv);
		{ auto output = Output(app); }
		{ auto defaults = Defaults(app); }
		auto points = Points(app);
		auto mesh = Mesh(app, points);
		auto edges = Edges(app, mesh);
		auto polygons = Polygons(app, edges);
		auto output = Output(app, polygons, points);

		std::exit(EXIT_SUCCESS);
	} catch (std::ios_base::failure &) {
		std::cerr << "error: problem reading or writing file" << std::endl;
		return EXIT_FAILURE;
	} catch (std::runtime_error &error) {
		std::cerr << "error: " << error.what() << std::endl;
		return EXIT_FAILURE;
	}
}
