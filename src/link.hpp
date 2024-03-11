////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LINK_HPP
#define LINK_HPP

#include "vertex.hpp"
#include "exact.hpp"
#include <utility>
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include <compare>
#include <functional>
#include <cstddef>

using Link = std::pair<Vertex, Vertex>;
using Links = std::vector<Link>;

// link <  vertex : link lies to the left of vertex
// link <= vertex : link lies to the left of vertex or is colinear
// link >= vertex : link lies to the right of vertex or is colinear
// link >  vertex : link lies to the right of vertex

auto operator<=>(Link const &link, Vertex const &vertex) {
	auto static constexpr epsilon = 0.5 * std::numeric_limits<double>::epsilon();
	auto static constexpr error_scale = epsilon * (3 + 16 * epsilon);

	auto const &[v1, v2] = link;
	auto const &[x1, y1] = v1;
	auto const &[x2, y2] = v2;
	auto const &[x3, y3] = vertex;

	auto const det1 = (x2 - x1) * (y3 - y2);
	auto const det2 = (x3 - x2) * (y2 - y1);
	auto const det = det1 - det2;

	if (std::abs(det) > error_scale * (std::abs(det1) + std::abs(det2)))
		return det <=> 0;

	auto const [x_min, x_max] = std::minmax({x1, x2, x3});
	auto const [y_min, y_max] = std::minmax({y1, y2, y3});

	if ((2 * x_min > x_max || 2 * x_max < x_min) && (2 * y_min > y_max || 2 * y_max < y_min)) {
		auto const det1 = Exact(x2 - x1) * Exact(y3 - y2);
		auto const det2 = Exact(x3 - x2) * Exact(y2 - y1);
		return det1 - det2 <=> 0;
	} else {
		auto const det1 = Exact(x1) * Exact(y2) - Exact(x2) * Exact(y1);
		auto const det2 = Exact(x2) * Exact(y3) - Exact(x3) * Exact(y2);
		auto const det3 = Exact(x3) * Exact(y1) - Exact(x1) * Exact(y3);
		return det1 + det2 + det3 <=> 0;
	}
}

template <> struct std::hash<Link> {
	std::size_t operator()(Link const &link) const {
		auto static constexpr hash = std::hash<Vertex>();
		auto const seed = hash(link.first);
		return seed ^ (hash(link.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
