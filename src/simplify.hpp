////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLIFY_HPP
#define SIMPLIFY_HPP

#include "ring.hpp"
#include "rtree.hpp"
#include "bounds.hpp"
#include "polygons.hpp"
#include <utility>
#include <cmath>
#include <set>

class Simplify {
	using Corner = Ring::CornerIterator;
	using Ordinal = std::pair<bool, double>;

	template <bool erode>
	struct OneSided {
		struct Candidate {
			Corner corner;
			Bounds bounds;
			Ordinal ordinal;

			static auto order(Corner const &corner, RTree<Corner> const &rtree) {
				auto const cross = corner.cross();
				if (erode != (cross < 0))
					return Ordinal(false, std::abs(cross));
				auto const [v0, v1, v2] = *corner;
				auto const bounds = Bounds(v0, v2);
				for (auto const &corner: rtree.search(bounds)) {
					auto const [u0, u1, u2] = *corner;
					// if (/* u0-u1 intersects with v0-v2 */)
					// 	return = Ordinal(false, std::abs(cross));
					// if (/* u1-u2 intersects with v0-v2 */)
					// 	return = Ordinal(false, std::abs(cross));
				}
				return Ordinal(true, std::abs(cross));
			}

			Candidate(Corner const &corner, RTree<Corner> const &rtree) : corner(corner), bounds(corner.bounds()), ordinal(order(corner, rtree)) { }

			friend auto operator<(Candidate const &candidate1, Candidate const &candidate2) {
				return candidate1.ordinal < candidate2.ordinal;
			}

			friend auto operator==(Candidate const &candidate1, Candidate const &candidate2) {
				return candidate1.corner == candidate2.corner;
			}

			auto operator<(double corner_area) const {
				return ordinal < Ordinal(false, corner_area * 2);
			}
		};

		void operator()(Polygons &polygons, double tolerance) {
			auto queue = std::multiset<Candidate>();
			auto corners = std::vector<Corner>();
			for (auto &polygon: polygons)
				for (auto &ring: polygon)
					for (auto corner = ring.begin(); corner != ring.end(); ++corner)
						corners.push_back(corner);
			auto rtree = RTree(corners);
			for (auto &polygon: polygons)
				for (auto &ring: polygon)
					for (auto corner = ring.begin(); corner != ring.end(); ++corner)
						queue.emplace(corner, rtree);
			while (!queue.empty() && *queue.begin() < tolerance) {
				auto const least = queue.begin();
				auto const corner = least->corner;
				auto const bounds = least->bounds;
				queue.erase(least);
				if (corner.ring_size() <= 4)
					continue;
				auto const next = corner.next();
				auto const prev = corner.prev();
				for (auto const &corner: rtree.search(bounds)) {
					auto const candidate = Candidate(corner, rtree);
					auto const [begin, end] = queue.equal_range(candidate);
					queue.erase(std::find(begin, end, candidate));
				};
				rtree.erase(corner);
				corner.remove();
				rtree.update(next);
				rtree.update(prev);
				for (auto const &corner: rtree.search(bounds))
					queue.emplace(corner, rtree);
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
