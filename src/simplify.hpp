////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLIFY_HPP
#define SIMPLIFY_HPP

#include "ring.hpp"
#include "polygons.hpp"
#include <utility>
#include <cmath>
#include <set>

class Simplify {
	using Corner = Ring::CornerIterator;

	template <bool erode>
	struct OneSided {
		struct Compare {
			auto operator()(Corner const &corner) const {
				auto const cross = corner.cross();
				return std::pair(erode == (cross < 0), std::abs(cross));
			}

			auto operator()(Corner const &corner1, Corner const &corner2) const {
				return (*this)(corner1) < (*this)(corner2);
			}

			auto operator()(double corner_area) const {
				return std::pair(false, 2 * corner_area);
			}
		};

		void operator()(Polygons &polygons, double tolerance) {
			auto const compare = Compare();
			auto const limit = compare(tolerance);
			auto corners = std::multiset<Corner, Compare>(compare);
			for (auto &polygon: polygons)
				for (auto &ring: polygon)
					for (auto corner = ring.begin(); corner != ring.end(); ++corner)
						corners.insert(corner);
			while (!corners.empty() && compare(*corners.begin()) < limit) {
				auto const least = corners.begin();
				auto const corner = *least;
				auto const prev = corner.prev();
				auto const next = corner.next();
				corners.erase(least);
				if (corner.ring_size() > 4) {
					corners.erase(prev);
					corners.erase(next);
					corner.remove();
					corners.insert(next.prev());
					corners.insert(prev.next());
				}
			}
		}
	};

public:
	void operator()(Polygons &polygons, double tolerance, bool open) {
		if (open) {
			OneSided<false>()(polygons, tolerance);
			OneSided< true>()(polygons, tolerance);
		} else {
			OneSided< true>()(polygons, tolerance);
			OneSided<false>()(polygons, tolerance);
		}
	}
};

#endif
