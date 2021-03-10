#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP

#include "triangle.hpp"
#include "edge.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cstddef>
#include <array>
#include <cmath>

class Triangles {
	std::unordered_set<Triangle> triangles;
	std::unordered_map<Edge, Triangle> neighbours;

	Triangles(Triangles *source) {
		std::unordered_set<Triangle> pending;
		for (pending.insert(*source->triangles.begin()); !pending.empty(); ) {
			const auto &triangle = *pending.begin();
			*source -= triangle;
			*this += triangle;
			for (const auto edge: triangle) {
				const auto pair = source->neighbours.find(edge);
				if (pair != source->neighbours.end())
					pending.insert(pair->second);
			}
			pending.erase(triangle);
		}
	}

public:
	Triangles() { }

	auto begin() const { return triangles.begin(); }
	auto   end() const { return triangles.end(); }

	Triangles &operator+=(const Triangle &triangle) {
		triangles.insert(triangle);
		for (const auto edge: triangle)
			neighbours.emplace(-edge, triangle);
		return *this;
	}

	Triangles &operator-=(const Triangle &triangle) {
		triangles.erase(triangle);
		for (const auto edge: triangle)
			neighbours.erase(-edge);
		return *this;
	}

	template <typename Function>
	auto explode(Function function) {
		while (!triangles.empty())
			function(Triangles(this));
	}

	auto operator>(double length) const {
		return std::any_of(triangles.begin(), triangles.end(), [=](const auto &triangle) {
			return triangle > length;
		});
	}

	auto is_water(double height, double slope, bool permissive) const {
		Vector<3> perp;
		double sum_abs = 0.0;
		std::size_t count = 0;

		for (const auto &triangle: triangles) {
			auto edge = triangle.begin();
			std::array edges = {edge++, edge++, edge};
			std::iter_swap(edges.begin(), std::min_element(edges.begin(), edges.end(), [](const auto &edge1, const auto *edge2) {
				return *edge1 < *edge2;
			}));
			perp += *edges[1] % *edges[2];
			for (const auto &edge: {edges[1], edges[2]})
				if (edge->first.ground && edge->second.ground) {
					sum_abs += std::abs(edge->second[2] - edge->first[2]);
					++count;
				}
		}

		auto angle = std::acos(std::abs(perp.normalise()[2])) * 180.0 / M_PI;
		return angle > slope ? false : count < 3 ? permissive : sum_abs / count < height;
	}
};

#endif
