#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "point.hpp"
#include "edge.hpp"
#include "edges.hpp"
#include "ring.hpp"
#include "tin.hpp"
#include "ply.hpp"
#include "edges.hpp"
#include "faces.hpp"
#include <vector>
#include <ostream>
#include <utility>
#include <string>
#include <numeric>
// #include <algorithm>
// #include <iterator>

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

	static auto from_tiles(const std::vector<std::string> &tile_paths, double length, double width, double height, double slope, double area, double cell_size, bool strict) {
		Edges edges;
		Faces faces;

		std::accumulate(tile_paths.begin(), tile_paths.end(), TIN(), [&](auto &tin, const auto &tile_path) {
			return tin += PLY(tile_path, cell_size);
		}).triangulate().each_face([&](const auto &face) {
			edges += face;
			if (face > length)
				faces += face;
		});

		faces.explode([&](const auto &group) {
			if ((edges || group) || ((width <= length || group > width) && group.is_water(height, slope, strict)))
				for (const auto &face: group)
					edges -= face;
		});

		auto rings = edges.rings();
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
