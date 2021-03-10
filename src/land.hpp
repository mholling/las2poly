#ifndef LAND_HPP
#define LAND_HPP

#include "polygon.hpp"
#include "mesh.hpp"
#include "triangles.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include "ring.hpp"
#include <algorithm>
#include <vector>
#include <iterator>
#include <ostream>

struct Land : std::vector<Polygon> {
	Land(Mesh &mesh, double length, double width, double height, double slope, double area, bool permissive) {
		Triangles large_triangles;
		Edges outside_edges;

		mesh.deconstruct([&](const auto &triangle) {
			if (triangle > length)
				large_triangles += triangle;
		}, [&](const auto &edge) {
			outside_edges.insert(edge);
		});

		large_triangles.explode([&](const auto &triangles) {
			if ((outside_edges || triangles) || ((width <= length || triangles > width) && triangles.is_water(height, slope, permissive)))
				for (const auto &triangle: triangles)
					outside_edges -= triangle;
		});

		auto rings = Rings(outside_edges)();
		auto rings_end = std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		});
		auto holes_begin = std::partition(rings.begin(), rings_end, [](const auto &ring) {
			return ring > 0;
		});
		std::sort(rings.begin(), holes_begin);

		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&](const auto &exterior) {
			std::vector<Ring> rings = {exterior};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings_end, [&](const auto &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(rings));
			emplace_back(rings);
		});
	}

	friend auto &operator<<(std::ostream &json, const Land &land) {
		bool first = true;
		for (const auto &polygon: land)
			json << (std::exchange(first, false) ? '[' : ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		json << (first ? "[]" : "]");
		return json;
	}
};

#endif
