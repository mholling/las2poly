#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "point.hpp"
#include "ring.hpp"
#include "edges.hpp"
#include "faces.hpp"
#include <vector>
#include <ostream>
#include <string>
#include <fstream>
#include <cstddef>
#include <sstream>
#include <algorithm>
#include <iterator>

class Polygon {
	std::vector<Ring> rings;

public:
	Polygon(const std::vector<Ring> &rings) : rings(rings) { }

	friend std::ostream &operator<<(std::ostream &json, const Polygon &polygon) {
		unsigned int count = 0;
		for (const auto &ring: polygon.rings)
			json << (count++ ? ',' : '[') << ring;
		return json << ']';
	}

	static auto from_ply(const std::string &ply_path, double area, double length, double width, double noise, double slope, unsigned int consensus, unsigned int iterations) {
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

		std::vector<Point> points(vertex_count);
		for (std::size_t index = 0; index < vertex_count; ++index)
			points[index] = Point(ply, index);

		Edges edges;
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

		auto rings = edges.rings();
		rings.erase(std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		}), rings.end());

		std::vector<Ring> exteriors, holes;
		std::partition_copy(rings.begin(), rings.end(), std::back_inserter(exteriors), std::back_inserter(holes), [](const Ring &ring) {
			return ring > 0;
		});
		std::sort(exteriors.begin(), exteriors.end());

		std::vector<Polygon> polygons;
		auto remaining = holes.begin();
		for (const auto &exterior: exteriors) {
			std::vector<Ring> rings = {exterior};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, holes.end(), [&](const auto &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(rings));
			polygons.push_back(Polygon(rings));
		}

		return polygons;
	}
};

#endif
