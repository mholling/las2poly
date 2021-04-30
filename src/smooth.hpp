////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SMOOTH_HPP
#define SMOOTH_HPP

#include "ring.hpp"
#include "polygons.hpp"
#include <set>
#include <cmath>
#include <algorithm>

class Smooth {
	using Corner = Ring::CornerIterator;

	struct Compare {
		auto operator()(Corner const &corner) const {
			return corner.cosine();
		}

		auto operator()(Corner const &corner1, Corner const &corner2) const {
			return (*this)(corner1) < (*this)(corner2);
		}
	};

public:
	void operator()(Polygons &polygons, double tolerance, double angle) {
		auto corners = std::multiset<Corner, Compare>();
		for (auto &polygon: polygons)
			for (auto &ring: polygon)
				for (auto corner = ring.begin(); corner != ring.end(); ++corner)
					corners.insert(corner);
		for (auto const cosine = std::cos(angle); !corners.empty() && corners.begin()->cosine() < cosine; ) {
			auto const least = corners.begin();
			auto const corner = *least;
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const [v0, v1, v2] = *corner;
			auto const f0 = std::min(0.25, tolerance / (v1 - v0).norm());
			auto const f2 = std::min(0.25, tolerance / (v2 - v1).norm());
			auto const v10 = v0 * f0 + v1 * (1.0 - f0);
			auto const v11 = v2 * f2 + v1 * (1.0 - f2);
			corners.erase(least);
			corner.replace(v10, v11);
			corners.insert(next.prev());
			corners.insert(prev.next());
		}
	}
};

#endif
