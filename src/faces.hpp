#ifndef FACES_HPP
#define FACES_HPP

#include "face.hpp"
#include "edge.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

class Faces {
	std::unordered_set<Face> faces;
	std::unordered_map<Edge, Face> neighbours;

	Faces &operator+=(const Face &face) {
		faces.insert(face);
		for (const auto edge: face)
			neighbours.insert(std::pair(-edge, face));
		return *this;
	}

	Faces &operator-=(const Face &face) {
		faces.erase(face);
		for (const auto edge: face)
			neighbours.erase(-edge);
		return *this;
	}

	Faces(Faces *source) {
		std::unordered_set<Face> pending;
		for (pending.insert(*source->faces.begin()); !pending.empty(); ) {
			const auto &face = *pending.begin();
			*source -= face;
			*this += face;
			for (const auto edge: face) {
				const auto pair = source->neighbours.find(edge);
				if (pair != source->neighbours.end())
					pending.insert(pair->second);
			}
			pending.erase(face);
		}
	}

public:
	Faces() { }

	auto begin() const { return faces.begin(); }
	auto   end() const { return faces.end(); }

	template <typename Function>
	auto explode(Function function) {
		while (!faces.empty())
			function(Faces(this));
	}

	auto operator>(double length) const {
		return std::any_of(faces.begin(), faces.end(), [=](const auto &face) {
			return face > length;
		});
	}

	auto is_water(double height, double slope, bool strict) const {
		Vector<3> perp;
		double sum_abs = 0.0;
		std::size_t count = 0;

		for (const auto &face: faces) {
			auto edge = face.begin();
			std::array<Edge, 3> edges = {*edge++, *edge++, *edge++};
			std::iter_swap(edges.begin(), std::min_element(edges.begin(), edges.end()));
			perp += edges[1] % edges[2];
			for (const auto &edge: {edges[1], edges[2]})
				if (edge.first.is_ground() && edge.second.is_ground()) {
					sum_abs += std::abs(edge.second[2] - edge.first[2]);
					++count;
				}
		}

		auto angle = std::acos(std::abs(perp.normalise()[2])) * 180.0 / M_PI;
		return angle > slope ? false : count < 3 ? !strict : sum_abs / count < height;
	}
};

#endif