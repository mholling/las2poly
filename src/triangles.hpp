////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP

#include "triangle.hpp"
#include "edge.hpp"
#include "app.hpp"
#include "vector.hpp"
#include "summation.hpp"
#include <unordered_set>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <cmath>

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

	class Grouped {
		Triangles &triangles;
		Neighbours neighbours;

		struct Iterator {
			Triangles &triangles;
			Neighbours &neighbours;
			std::size_t remaining;

			Iterator(Triangles &triangles, Neighbours &neighbours, std::size_t remaining) :
				triangles(triangles),
				neighbours(neighbours),
				remaining(remaining)
			{ }

			Iterator(Triangles &triangles, Neighbours &neighbours) :
				Iterator(triangles, neighbours, triangles.size())
			{ }

			auto &operator++() {
				remaining = triangles.size();
				return *this;
			}

			auto operator==(Iterator const &other) const {
				return other.remaining == remaining;
			}

			auto operator!=(Iterator const &other) const {
				return other.remaining != remaining;
			}

			auto operator*() {
				return Triangles(triangles, neighbours);
			}
		};

	public:
		Grouped(Triangles &triangles) :
			triangles(triangles),
			neighbours(triangles)
		{ }

		auto begin() { return Iterator(triangles, neighbours); }
		auto   end() { return Iterator(triangles, neighbours, 0); }
	};

public:
	Triangles() = default;

	auto grouped() {
		return Grouped(*this);
	}

	auto is_water(App const &app) const {
		auto perp_sum = Vector<3>{{0.0, 0.0, 0.0}};
		auto perp_sum_z = Summation(perp_sum[2]);

		auto delta_sum = 0.0;
		auto delta_count = 0ul;
		auto delta_summer = Summation(delta_sum);

		for (auto const &[edge0, edge1, edge2]: *this) {
			auto const perp = edge1 ^ edge2;
			auto const &p0 = *edge0.first;
			auto const &p1 = *edge1.first;
			auto const &p2 = *edge2.first;

			if (p0.synthetic() || p1.synthetic() || p2.synthetic()) {
				perp_sum_z += perp.norm();
				delta_count += 2;
			} else if (p0.ground() && p1.ground() && p2.ground()) {
				perp_sum[0] += perp[0];
				perp_sum[1] += perp[1];
				perp_sum_z  += perp[2];
				delta_summer += std::abs(p1.elevation - p2.elevation);
				delta_summer += std::abs(p2.elevation - p0.elevation);
				delta_count += 2;
			}
		}

		return delta_sum < app.delta * delta_count && std::abs(perp_sum[2]) > app.min_cosine * perp_sum.norm();
	}
};

#endif
