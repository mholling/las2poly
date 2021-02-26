#ifndef MESH_HPP
#define MESH_HPP

#include "face.hpp"
#include "edge.hpp"
#include "rings.hpp"
#include "exterior.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstddef>
#include <array>
#include <algorithm>
#include <cmath>

class Mesh {
	std::unordered_set<Face, Face::Hash> faces;
	std::unordered_set<Edge, Edge::Hash> edges;
	std::unordered_map<Edge, Face, Edge::Hash> neighbours;

	Mesh(Mesh *source) {
		std::unordered_set<Face, Face::Hash> pending;
		for (pending.insert(*source->faces.begin()); !pending.empty(); ) {
			const auto &face = *pending.begin();
			*source -= face;
			*this += face;
			source->each_neighbour(face, [&](const auto &neighbour) {
				pending.insert(neighbour);
			});
			pending.erase(face);
		}
	}

public:
	Mesh() { }

	auto begin() const { return faces.begin(); }
	auto   end() const { return faces.end(); }

	Mesh &operator+=(const Face &face) {
		faces.insert(face);
		for (const auto edge: face.edges()) {
			if (!edges.erase(-edge))
				edges.insert(edge);
			neighbours.insert(std::make_pair(-edge, face));
		}
		return *this;
	}

	Mesh &operator-=(const Face &face) {
		faces.erase(face);
		for (const auto edge: face.edges()) {
			if (!edges.erase(edge))
				edges.insert(-edge);
			neighbours.erase(-edge);
		}
		return *this;
	}

	auto &operator+=(const Edge &edge) {
		edges.insert(edge);
		edges.insert(-edge);
		return *this;
	}

	auto &operator-=(const Edge &edge) {
		return *this -= neighbours.find(-edge)->second;
	}

	auto &operator+=(const Mesh &mesh) {
		for (const auto &face: mesh)
			*this += face;
		return *this;
	}

	auto &operator-=(const Mesh &mesh) {
		for (const auto &face: mesh)
			*this -= face;
		return *this;
	}

	auto rings() const {
		return Rings(edges)();
	}

	template <typename F>
	auto explode(F function) {
		while (!faces.empty())
			function(Mesh(this));
	}

	template <typename F>
	auto select(F function) {
		Mesh mesh;
		for (auto &face: faces)
			if (function(face))
				mesh += face;
		return mesh;
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

	auto operator||(const Mesh &mesh) const {
		for (const auto &edge: mesh.edges)
			if (edges.count(edge) > 0)
				return true;
		return false;
	}

	auto exterior() {
		return Exterior(edges, neighbours);
	}

	auto is_water(double height, double slope, bool strict) const {
		Vector<3> perp;
		double sum_abs = 0.0;
		std::size_t count = 0;

		for (const auto &face: faces) {
			auto edge = face.edges().begin();
			std::array<Edge, 3> edges = {*edge++, *edge++, *edge++};
			std::iter_swap(edges.begin(), std::min_element(edges.begin(), edges.end()));
			perp += edges[1].delta3d() ^ edges[2].delta3d();
			for (const auto &edge: {edges[1], edges[2]})
				if (edge.is_ground()) {
					sum_abs += std::abs(edge.p1[2] - edge.p0[2]);
					++count;
				}
		}

		auto angle = std::acos(std::abs(perp.normalise()[2])) * 180.0 / M_PI;
		return angle > slope ? false : count < 3 ? !strict : sum_abs / count < height;
	}
};

#endif
