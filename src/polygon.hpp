#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "mesh.hpp"
#include "edge.hpp"
#include "face.hpp"
#include "faces.hpp"
#include "edges.hpp"
#include "ring.hpp"
#include "faces.hpp"
#include <vector>
#include <ostream>
#include <utility>
#include <algorithm>
#include <iterator>

class Polygon {
	std::vector<Ring> rings;

public:
	Polygon(const std::vector<Ring> &rings) : rings(rings) { }

	friend std::ostream &operator<<(std::ostream &json, const Polygon &polygon) {
		bool first = true;
		for (const auto &ring: polygon.rings)
			json << (std::exchange(first, false) ? '[' : ',') << ring;
		return json << ']';
	}

	static auto from_mesh(Mesh &mesh, double length, double width, double height, double slope, double area, double cell_size, bool strict) {
		Faces large_faces;
		Edges outside_edges;

		mesh.deconstruct([&](const Face &face) {
			if (face > length)
				large_faces += face;
		}, [&](const Edge &edge) {
			outside_edges += edge;
		});

		large_faces.explode([&](const auto &faces) {
			if ((outside_edges || faces) || ((width <= length || faces > width) && faces.is_water(height, slope, strict)))
				for (const auto &face: faces)
					outside_edges -= face;
		});

		auto rings = outside_edges.rings();
		auto rings_end = std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		});
		auto holes_begin = std::partition(rings.begin(), rings_end, [](const Ring &ring) {
			return ring > 0;
		});
		std::sort(rings.begin(), holes_begin);

		std::vector<Polygon> polygons;
		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&](const auto &exterior) {
			std::vector<Ring> rings = {exterior};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings_end, [&](const auto &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(rings));
			polygons.push_back(Polygon(rings));
		});

		return polygons;
	}
};

#endif
