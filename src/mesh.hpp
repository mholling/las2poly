#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "edge.hpp"
#include "face.hpp"
#include "plane.hpp"
#include "estimator.hpp"
#include "ring.hpp"
#include "polygon.hpp"
#include <vector>
#include <unordered_set>
#include <string>
#include <fstream>
#include <cstddef>
#include <sstream>
#include <algorithm>

class Mesh {
	std::vector<Point> points;
	std::unordered_set<Face, Face::Hash> faces;

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
			faces.insert(Face(points, ply, index));
	}

	void remove_voids(double noise, double length, double slope, unsigned int consensus, unsigned int iterations) {
		std::unordered_set<Face, Face::Hash> gaps;
		std::copy_if(faces.begin(), faces.end(), std::inserter(gaps, gaps.begin()), [=](const auto &face) {
			return face > length;
		});

		Estimator<Plane, Point, 3> estimate(noise, consensus, iterations);
		Face::each_group(gaps, [&](const auto &group) {
			std::unordered_set<Point, Point::Hash> ground_points;
			Plane plane;

			for (const auto &face: group)
				for (const auto &vertex: face)
					if (vertex.is_ground())
						ground_points.insert(vertex);

			if (estimate(ground_points, plane))
				if (plane.slope() < slope)
					for (const auto &face: group)
						faces.erase(face);
		});
	}

	auto polygons(double area) const {
		auto edges = Face::external_edges(faces);
		auto rings = Ring::from_edges(edges);
		rings.erase(std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		}), rings.end());
		return Polygon::from_rings(rings);
	}
};

#endif
