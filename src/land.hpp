#ifndef LAND_HPP
#define LAND_HPP

#include "polygon.hpp"
#include "mesh.hpp"
#include "faces.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include "ring.hpp"
#include <algorithm>
#include <vector>
#include <iterator>
#include <ostream>

struct Land : std::vector<Polygon> {
	Land(Mesh &mesh, double length, double width, double height, double slope, double area, double cell_size, bool strict) {
		Faces large_faces;
		Edges outside_edges;

		mesh.deconstruct([&](const auto &face) {
			if (face > length)
				large_faces += face;
		}, [&](const auto &edge) {
			outside_edges += edge;
		});

		large_faces.explode([&](const auto &faces) {
			if ((outside_edges || faces) || ((width <= length || faces > width) && faces.is_water(height, slope, strict)))
				for (const auto &face: faces)
					outside_edges -= face;
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

	friend std::ostream &operator<<(std::ostream &json, const Land &land) {
		bool first = true;
		for (const auto &polygon: land)
			json << (std::exchange(first, false) ? '[' : ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		json << (first ? "[]" : "]");
		return json;
	}
};

#endif
