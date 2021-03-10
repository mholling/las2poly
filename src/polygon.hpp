#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "ring.hpp"
#include <vector>
#include <ostream>
#include <utility>

using Polygon = std::vector<Ring>;

auto &operator<<(std::ostream &json, const Polygon &polygon) {
	auto separator = '[';
	for (const auto &ring: polygon)
		json << std::exchange(separator, ',') << ring;
	return json << ']';
}

#endif
