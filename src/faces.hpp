#ifndef FACES_HPP
#define FACES_HPP

#include "point.hpp"
#include "face.hpp"
#include "rtree.hpp"
#include "plane.hpp"
#include "estimator.hpp"
#include <unordered_set>
#include <queue>
#include <vector>

class Faces {
	std::unordered_set<Face, Face::Hash> faces;

	template <typename F>
	Faces(F &source, const RTree<Face> &rtree) {
		std::queue<Face> pending;
		for (pending.push(*source.begin()); !pending.empty(); pending.pop()) {
			const auto &face = pending.front();
			source.erase(face);
			faces.insert(face);
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

	Faces() { }

	auto separate() {
		std::vector<Faces> results;
		RTree<Face> rtree(faces);
		while (!faces.empty())
			results.push_back(Faces(*this, rtree));
		return results;
	}

	void insert(const Face &face) {
		faces.insert(face);
	}

	void erase(const Face &face) {
		faces.erase(face);
	}

	auto operator>(double length) const {
		for (const auto &face: faces)
			if (face > length)
				return true;
		return false;
	}

	auto is_water(double noise, double slope, unsigned int consensus, unsigned int iterations) const {
		std::unordered_set<Point, Point::Hash> ground_points;
		for (const auto &face: faces)
			for (const auto &vertex: face)
				if (vertex.is_ground())
					ground_points.insert(vertex);

		Estimator<Plane, Point, 3> estimate(noise, consensus, iterations);
		Plane plane;
		return estimate(ground_points, plane) && plane.slope() < slope;
	}
};

#endif
