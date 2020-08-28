#ifndef MESH_HPP
#define MESH_HPP

#include "point.hpp"
#include "face.hpp"
#include "edge.hpp"
#include "rtree.hpp"
#include "connections.hpp"
#include "plane.hpp"
#include "estimator.hpp"
#include <unordered_set>
#include <queue>
#include <fstream>
#include <cstddef>
#include <vector>

class Mesh {
	std::unordered_set<Face, Face::Hash> faces;
	std::unordered_set<Edge, Edge::Hash> edges;

	template <typename M>
	Mesh(M &mesh, const RTree<Face> &rtree) {
		std::queue<Face> pending;
		for (pending.push(*mesh.begin()); !pending.empty(); pending.pop()) {
			const auto &face = pending.front();
			mesh.erase(face);
			insert(face);
			rtree.search(face, [&](const auto &other) {
				if (faces.count(other))
					return;
				if (face || other)
					pending.push(other);
			});
		}
	}

public:
	auto begin() const { return faces.begin(); }
	auto   end() const { return faces.end(); }

	Mesh() { }

	template <typename C>
	Mesh(const C &points, std::ifstream &input, std::size_t count) {
		faces.reserve(count);
		for (std::size_t index = 0; index < count; ++index)
			insert(Face(points, input, index));
	}

	auto separate() {
		std::vector<Mesh> results;
		RTree<Face> rtree(faces);
		while (!faces.empty())
			results.push_back(Mesh(*this, rtree));
		return results;
	}

	void insert(const Face &face) {
		faces.insert(face);
		for (const auto edge: face.edges())
			if (!edges.erase(-edge))
				edges.insert(edge);
	}

	void erase(const Face &face) {
		faces.erase(face);
		for (const auto edge: face.edges())
			if (!edges.erase(edge))
				edges.insert(-edge);
	}

	auto operator>(double length) const {
		for (const auto &face: *this)
			if (face > length)
				return true;
		return false;
	}

	auto rings() const {
		return Connections(edges).rings();
	}

	auto is_water(double noise, double slope, unsigned int consensus, unsigned int iterations) const {
		std::unordered_set<Point, Point::Hash> ground_points;
		for (const auto &face: *this)
			for (const auto &vertex: face)
				if (vertex.is_ground())
					ground_points.insert(vertex);

		Estimator<Plane, Point, 3> estimate(noise, consensus, iterations);
		Plane plane;
		return estimate(ground_points, plane) && plane.slope() < slope;
	}
};

#endif
