////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLIFY_HPP
#define SIMPLIFY_HPP

#include "corner.hpp"
#include "ring.hpp"
#include "bounds.hpp"
#include "rtree.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <vector>

template <typename Polygons>
class Simplify {
	auto static constexpr min_ring_size = 8;
	using Corner = Corner<Ring>;

	struct Candidate {
		Corner corner;
		Bounds bounds;
		double cross;

		Candidate(Corner const &corner) :
			corner(corner),
			bounds(corner),
			cross(corner.cross())
		{ }

		friend auto operator==(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.corner == candidate2.corner;
		}

		friend auto operator<(Candidate const &candidate1, Candidate const &candidate2) {
			return std::abs(candidate1.cross) < std::abs(candidate2.cross);
		}

		template <typename RTree>
		auto removeable(RTree const &rtree, double corner_area, bool erode) const {
			if (cross == 0)
				return true;
			if (erode == (cross < 0) || std::abs(cross) > corner_area * 2 || corner.ring_size() <= min_ring_size)
				return false;
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const &v0 = prev.vertex();
			auto const &v1 = corner.vertex();
			auto const &v2 = next.vertex();
			auto search = rtree.search(bounds);
			return std::none_of(search.begin(), search.end(), [&](auto const &other) {
				if (other == corner)
					return false;
				auto const &[u0, u1, u2] = other;
				if (other == prev)
					return u0 == v2;
				if (other == next)
					return u2 == v0;
				auto const cross01 = (u1 - v0) ^ (u1 - v1);
				auto const cross12 = (u1 - v1) ^ (u1 - v2);
				auto const cross20 = (u1 - v2) ^ (u1 - v0);
				if (cross01 < 0 && cross12 < 0 && cross20 < 0)
					return true;
				if (cross01 > 0 && cross12 > 0 && cross20 > 0)
					return true;
				if (u1 == v1)
					for (auto const &u: {u0, u2}) {
						auto const cross01 = (u - v0) ^ (u - v1);
						auto const cross12 = (u - v1) ^ (u - v2);
						if (cross01 < 0 && cross12 < 0 && cross < 0)
							return true;
						if (cross01 > 0 && cross12 > 0 && cross > 0)
							return true;
					}
				return false;
			});
		}

		template <typename RTree>
		void erase(RTree &rtree) const {
			auto const next = corner.next();
			auto const prev = corner.prev();
			auto const next_bounds = Bounds(next);
			auto const prev_bounds = Bounds(prev);
			corner.erase();
			rtree.update(next, next_bounds);
			rtree.update(prev, prev_bounds);
		}
	};

	using Ordered = std::multiset<Candidate>;
	using Corners = std::vector<Corner>;

	void simplify_one_sided(double tolerance, bool erode, int threads) {
		auto corners = Corners();
		auto ordered = Ordered();
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto corner: ring.corners())
					corners.push_back(corner);
		auto rtree = RTree(corners, threads);
		for (auto const &corner: corners)
			if (auto const candidate = Candidate(corner); candidate.removeable(rtree, tolerance, erode))
				ordered.insert(candidate);
		while (!ordered.empty()) {
			auto const least = ordered.begin();
			auto const candidate = *least;
			ordered.erase(least);
			if (candidate.corner.ring_size() <= min_ring_size)
				continue;
			rtree.erase(candidate.corner, candidate.bounds);
			auto search = rtree.search(candidate.bounds);
			auto const updates = Corners(search.begin(), search.end());
			for (auto const &corner: updates) {
				auto const candidate = Candidate(corner);
				auto const [begin, end] = ordered.equal_range(candidate);
				auto const position = std::find(begin, end, candidate);
				if (position != end)
					ordered.erase(position);
			}
			candidate.erase(rtree);
			for (auto const &corner: updates)
				if (auto const candidate = Candidate(corner); candidate.removeable(rtree, tolerance, erode))
					ordered.insert(candidate);
		}
	}

public:
	void simplify(double tolerance, bool open, int threads) {
		simplify_one_sided(tolerance, !open, threads);
		simplify_one_sided(tolerance, open, threads);
	}
};

#endif
