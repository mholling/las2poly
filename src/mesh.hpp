#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "edge.hpp"
#include "face.hpp"
#include "group.hpp"
#include "ring.hpp"
#include "polygon.hpp"
#include <vector>
#include <unordered_set>
#include <string>
#include <fstream>
#include <cstddef>
#include <sstream>
#include <algorithm>
#include <iterator>

class Mesh {
	std::vector<Point> points;
	std::unordered_set<Face, Face::Hash> faces;
	std::unordered_set<Edge, Edge::Hash> edges;

	void insert(const Face &face) {
		faces.insert(face);
		for (const auto edge: face.edges())
			if (!edges.erase(edge.opposite()))
				edges.insert(edge);
	}

	void erase(const Face &face) {
		faces.erase(face);
		for (const auto edge: face.edges())
			if (!edges.erase(edge))
				edges.insert(edge.opposite());
	}

public:
	Mesh(const std::string &ply_path) {
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

		faces.reserve(face_count);
		for (std::size_t index = 0; index < face_count; ++index)
			insert(Face(points, ply, index));
	}

	void remove_voids(double noise, double length, double width, double slope, unsigned int consensus, unsigned int iterations) {
		std::unordered_set<Face, Face::Hash> gaps;
		std::copy_if(faces.begin(), faces.end(), std::inserter(gaps, gaps.begin()), [=](const auto &face) {
			return face > length;
		});

		Group::each(gaps, [&](const auto &group) {
			if (group.adjoins(edges) || ((width <= length || group > width) && group.is_water(noise, slope, consensus, iterations)))
				for (const auto &face: group)
					erase(face);
		});
	}

	auto polygons(double area) const {
		auto rings = Ring::from_edges(edges);
		rings.erase(std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		}), rings.end());

		return Polygon::from_rings(rings);
	}
};

#endif
