////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include "vertex.hpp"
#include "bounds.hpp"
#include "exact.hpp"
#include <utility>
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include <compare>
#include <functional>
#include <cstddef>

using Segment = std::pair<Vertex, Vertex>;
using Segments = std::vector<Segment>;

template <>
Bounds::Bounds(Segment const &segment) {
	auto const &[v0, v1] = segment;
	auto const &[x0, y0] = v0;
	auto const &[x1, y1] = v1;
	std::tie(xmin, xmax) = std::minmax({x0, x1});
	std::tie(ymin, ymax) = std::minmax({y0, y1});
}

// segment <  vertex : segment lies to the left of vertex
// segment <= vertex : segment lies to the left of vertex or is colinear
// segment >= vertex : segment lies to the right of vertex or is colinear
// segment >  vertex : segment lies to the right of vertex

auto operator<=>(Segment const &segment, Vertex const &vertex) {
	auto static constexpr epsilon = 0.5 * std::numeric_limits<double>::epsilon();
	auto static constexpr error_scale = epsilon * (3 + 16 * epsilon);

	auto const &[v1, v2] = segment;
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

auto operator&(Segment const &u0u1, Segment const &v0v1) {
	auto const &[u0, u1] = u0u1;
	auto const &[v0, v1] = v0v1;

	auto const u0u1_v0 = u0u1 <=> v0;
	auto const u0u1_v1 = u0u1 <=> v1;

	if (u0u1_v0 == std::partial_ordering::equivalent)
		if (u0u1_v1 == std::partial_ordering::equivalent)
			return Bounds(u0u1) & Bounds(v0v1);

	auto const v0v1_u0 = v0v1 <=> u0;
	auto const v0v1_u1 = v0v1 <=> u1;

	return u0u1_v0 != u0u1_v1 && v0v1_u0 != v0v1_u1;
}

template <> struct std::hash<Segment> {
	std::size_t operator()(Segment const &segment) const {
		auto static constexpr hash = std::hash<Vertex>();
		auto const seed = hash(segment.first);
		return seed ^ (hash(segment.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}
};

#endif
