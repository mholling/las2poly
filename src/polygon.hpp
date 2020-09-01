#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "ply.hpp"
#include "ring.hpp"
#include "edges.hpp"
#include "faces.hpp"
#include <vector>
#include <ostream>
#include <utility>
#include <string>
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

	static auto from_ply(const std::string &ply_path, double length, double width, double height, double slope, double area, unsigned char klass, bool strict) {
		Edges edges;
		Faces gaps;
		PLY(ply_path).each_face([&](const auto face) {
			edges.insert(face);
			if (face > length)
				gaps.insert(face);
		});

		for (const auto &gap: gaps.separate())
			if ((gap && edges) || ((width <= length || gap > width) && gap.is_water(height, slope, klass, strict)))
				for (const auto &face: gap)
					edges.erase(face);

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
