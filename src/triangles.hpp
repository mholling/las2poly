////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP

#include "triangle.hpp"
#include "edge.hpp"
#include <unordered_set>
#include <unordered_map>
#include <iterator>
#include <cstddef>

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
			using iterator_category = std::input_iterator_tag;
			using value_type        = Triangles;
			using reference         = void;
			using pointer           = void;
			using difference_type   = void;

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
};

#endif
