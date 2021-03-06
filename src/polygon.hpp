#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "ring.hpp"
#include <vector>
#include <ostream>
#include <utility>

using Polygon = std::vector<Ring>;

std::ostream &operator<<(std::ostream &json, const Polygon &polygon) {
	bool first = true;
	for (const auto &ring: polygon)
		json << (std::exchange(first, false) ? '[' : ',') << ring;
	return json << ']';
}

#endif
