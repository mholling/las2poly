#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP

#include "triangle.hpp"
#include "edge.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <algorithm>
#include <unordered_map>

struct Triangles : std::unordered_set<Triangle> {
	Triangles() = default;

	auto &operator+=(const Triangle &triangle) {
		insert(triangle);
		for (const auto &edge: triangle)
			neighbours.emplace(-edge, triangle);
		return *this;
	}

	auto &operator-=(const Triangle &triangle) {
		erase(triangle);
		for (const auto &edge: triangle)
			neighbours.erase(-edge);
		return *this;
	}

	template <typename Function>
	void explode(Function function) {
		while (!empty())
			function(Triangles(this));
	}

	auto operator>(double length) const {
		return std::any_of(begin(), end(), [=](const auto &triangle) {
			return triangle > length;
		});
	}

private:
	std::unordered_map<Edge, Triangle> neighbours;

	Triangles(Triangles *source) {
		std::unordered_set<Triangle> pending;
		for (pending.insert(*source->begin()); !pending.empty(); ) {
			const auto &triangle = *pending.begin();
			*source -= triangle;
			*this += triangle;
			for (const auto &edge: triangle) {
				const auto pair = source->neighbours.find(edge);
				if (pair != source->neighbours.end())
					pending.insert(pair->second);
			}
			pending.erase(triangle);
		}
	}
};

#endif
