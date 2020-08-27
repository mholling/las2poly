#ifndef GROUP_HPP
#define GROUP_HPP

#include "point.hpp"
#include "face.hpp"
#include "rtree.hpp"
#include "plane.hpp"
#include "estimator.hpp"
#include <unordered_set>
#include <queue>

class Group {
	std::unordered_set<Face, Face::Hash> group;

	template <typename C>
	Group(C &faces, const RTree<Face> &rtree) {
		std::queue<Face> pending;
		for (pending.push(*faces.begin()); !pending.empty(); pending.pop()) {
			const auto &face = pending.front();
			faces.erase(face);
			group.insert(face);
			rtree.search(face, [&](const auto &other) {
				if (group.count(other))
					return;
				if (face || other)
					pending.push(other);
			});
		}
	}

public:
	auto begin() const { return group.begin(); }
	auto   end() const { return group.end(); }

	template <typename C, typename F>
	static void each(C &faces, F function) {
		RTree<Face> rtree(faces);
		while (!faces.empty())
			function(Group(faces, rtree));
	}

	auto operator>(double length) const {
		for (const auto &face: *this)
			if (face > length)
				return true;
		return false;
	}

	template <typename C>
	auto adjoins(const C &edges) const {
		for (const auto &face: *this)
			for (const auto edge: face.edges())
				if (edges.count(edge))
					return true;
		return false;
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
