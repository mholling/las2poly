#ifndef FACES_HPP
#define FACES_HPP

#include "point.hpp"
#include "face.hpp"
#include "edge.hpp"
#include "edges.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstddef>
#include <array>
#include <algorithm>
#include <cmath>

class Faces {
	std::unordered_set<Face, Face::Hash> faces;
	std::unordered_map<Edge, Face, Edge::Hash> neighbours;

	template <typename F>
	Faces(F &source) {
		std::unordered_set<Face, Face::Hash> pending;
		for (pending.insert(*source.begin()); !pending.empty(); ) {
			const auto &face = *pending.begin();
			source.erase(face);
			insert(face);
			source.each_neighbour(face, [&](const auto &neighbour) {
				pending.insert(neighbour);
			});
			pending.erase(face);
		}
	}

public:
	auto begin() const { return faces.begin(); }
	auto   end() const { return faces.end(); }

	Faces() { }

	void insert(const Face &face) {
		faces.insert(face);
		for (const auto edge: face.edges())
			neighbours.insert(std::make_pair(-edge, face));
	}

	void erase(const Face &face) {
		faces.erase(face);
		for (const auto edge: face.edges())
			neighbours.erase(-edge);
	}

	template <typename F>
	auto explode(F function) {
		while (!faces.empty())
			function(Faces(*this));
	}

	template <typename F>
	void each_neighbour(const Face &face, F function) {
		for (const auto edge: face.edges()) {
			const auto pair = neighbours.find(edge);
			if (pair != neighbours.end())
				function(pair->second);
		}
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
				if (edge.is_ground(klass)) {
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
