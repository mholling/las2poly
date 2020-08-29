#ifndef PLY_HPP
#define PLY_HPP

#include "point.hpp"
#include "edges.hpp"
#include "faces.hpp"
#include "polygon.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cstddef>
#include <sstream>
#include <algorithm>

class PLY {
	std::vector<Point> points;
	Edges edges;

public:
	PLY(const std::string &ply_path, double length, double width, double noise, double slope, unsigned int consensus, unsigned int iterations) {
		std::ifstream ply;
		ply.exceptions(ply.exceptions() | std::ifstream::failbit);
		ply.open(ply_path, std::ios::binary);
		std::size_t vertex_count, face_count;

		for (;;) {
			std::string line, command, type;
			std::getline(ply, line);
			std::istringstream words(line);
			words >> command;
			if ("end_header" == command)
				break;
			if ("element" != command)
				continue;
			words >> type;
			words >> (type == "vertex" ? vertex_count : face_count);
		}

		points.reserve(vertex_count);
		for (std::size_t index = 0; index < vertex_count; ++index)
			points.push_back(Point(ply, index));

		Faces gaps;
		for (std::size_t index = 0; index < face_count; ++index) {
			auto face = Face(points, ply, index);
			edges.insert(face);
			if (face > length)
				gaps.insert(face);
		}

		for (const auto &gap: gaps.separate())
			if ((width <= length || gap > width) && gap.is_water(noise, slope, consensus, iterations))
				for (const auto &face: gap)
					edges.erase(face);
	}

	auto polygons(double area) const {
		auto rings = edges.rings();
		rings.erase(std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		}), rings.end());

		return Polygon::from_rings(rings);
	}
};

#endif
