#ifndef FACES_HPP
#define FACES_HPP

#include "point.hpp"
#include "face.hpp"
#include "rtree.hpp"
#include "edges.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <queue>
#include <vector>
#include <cstddef>
#include <array>
#include <algorithm>
#include <cmath>

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

	auto operator&&(const Edges &edges) const {
		for (const auto &face: faces)
			for (const auto edge: face.edges())
				if (edges && edge)
					return true;
		return false;
	}

	auto is_water(double height, double slope, unsigned char klass, bool strict) const {
		Vector<3> perp;
		double square_sum = 0.0;
		std::size_t count = 0;

		for (const auto &face: faces) {
			auto edge = face.edges().begin();
			std::array<Edge, 3> edges = {*edge++, *edge++, *edge++};
			std::rotate(edges.begin(), std::min_element(edges.begin(), edges.end()), edges.end());
			perp += edges[1].delta3d() ^ edges[2].delta3d();
			for (const auto &edge: {edges[1], edges[2]})
				if (edge.vegetation(klass)) {
					auto delta_z = edge.p1[2] - edge.p0[2];
					square_sum += delta_z * delta_z;
					++count;
				}
		}

		auto angle = std::acos(std::abs(perp.normalise()[2])) * 180.0 / M_PI;
		return angle > slope ? false : count < 3 ? !strict : square_sum / count < height * height;
	}
};

#endif
