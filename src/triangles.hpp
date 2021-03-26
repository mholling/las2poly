#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP

#include "triangle.hpp"
#include "edge.hpp"
#include "vector.hpp"
#include <unordered_set>
#include <algorithm>
#include <unordered_map>

class Triangles : public std::unordered_set<Triangle> {
	struct Neighbours : std::unordered_map<Edge, Triangle> {
		Neighbours(const Triangles &triangles) {
			for (const auto &triangle: triangles)
				for (const auto &edge: triangle)
					emplace(-edge, triangle);
		}

		void erase(const Triangle &triangle) {
			for (const auto &edge: triangle)
				unordered_map::erase(-edge);
		}
	};

	Triangles(Triangles &source, Neighbours &neighbours) {
		std::unordered_set<Triangle> pending;
		for (pending.insert(*source.begin()); !pending.empty(); ) {
			const auto &triangle = *pending.begin();
			source.erase(triangle);
			neighbours.erase(triangle);
			insert(triangle);
			for (const auto &edge: triangle) {
				const auto pair = neighbours.find(edge);
				if (pair != neighbours.end())
					pending.insert(pair->second);
			}
			pending.erase(triangle);
		}
	}

public:
	Triangles() = default;

	template <typename Function>
	void explode(Function function) {
		auto neighbours = Neighbours(*this);
		while (!empty())
			function(Triangles(*this, neighbours));
	}

	auto operator>(double length) const {
		return std::any_of(begin(), end(), [=](const auto &triangle) {
			return triangle > length;
		});
	}
};

#endif
