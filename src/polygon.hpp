#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "ring.hpp"
#include <vector>
#include <ostream>
#include <algorithm>
#include <iterator>
#include <functional>

class Polygon {
	std::vector<Ring> rings;

public:
	Polygon(const std::vector<Ring> &rings) : rings(rings) { }

	friend std::ostream &operator<<(std::ostream &json, const Polygon &polygon) {
		unsigned int count = 0;
		for (const auto &ring: polygon.rings)
			json << (count++ ? ',' : '[') << ring;
		return json << ']';
	}

	static auto from_rings(const std::vector<Ring> &rings) {
		std::vector<Ring> exteriors, holes;
		std::vector<Polygon> polygons;

		std::partition_copy(rings.begin(), rings.end(), std::back_inserter(exteriors), std::back_inserter(holes), [](const Ring &ring) {
			return ring > 0;
		});
		std::sort(exteriors.begin(), exteriors.end());

		auto remaining = holes.begin();
		for (const auto &exterior: exteriors) {
			std::vector<Ring> rings = {exterior};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, holes.end(), [&](const auto &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(rings));
			polygons.push_back(Polygon(rings));
		}

		return polygons;
	}
};

#endif
