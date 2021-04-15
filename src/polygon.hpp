////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distrubuted under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "ring.hpp"
#include <vector>
#include <ostream>
#include <utility>

using Polygon = std::vector<Ring>;

auto &operator<<(std::ostream &json, Polygon const &polygon) {
	auto separator = '[';
	for (auto const &ring: polygon)
		json << std::exchange(separator, ',') << ring;
	return json << ']';
}

#endif
