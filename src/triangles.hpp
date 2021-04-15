////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distrubuted under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

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
		Neighbours(Triangles const &triangles) {
			for (auto const &triangle: triangles)
				for (auto const &edge: triangle)
					emplace(-edge, triangle);
		}

		void erase(Triangle const &triangle) {
			for (auto const &edge: triangle)
				unordered_map::erase(-edge);
		}
	};

	Triangles(Triangles &source, Neighbours &neighbours) {
		auto pending = std::unordered_set<Triangle>();
		for (pending.insert(*source.begin()); !pending.empty(); ) {
			auto const &triangle = *pending.begin();
			source.erase(triangle);
			neighbours.erase(triangle);
			insert(triangle);
			for (auto const &edge: triangle)
				if (auto const pair = neighbours.find(edge); pair != neighbours.end())
					pending.insert(pair->second);
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
		return std::any_of(begin(), end(), [=](auto const &triangle) {
			return triangle > length;
		});
	}
};

#endif
