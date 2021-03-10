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
};

#endif
