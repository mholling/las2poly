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
#include "segment.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <vector>

template <typename Polygons>
class Simplify {
	auto static constexpr min_ring_size = 8;
	using Corner = ::Corner<Ring>;
	using RTree = ::RTree<Corner>;

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

		auto operator()(RTree const &rtree, double corner_area, bool erode) const {
			if (cross == 0) return true;
			if (erode == (cross < 0)) return false;
			if (std::abs(cross) > corner_area * 2) return false;
			if (corner.ring_size() <= min_ring_size) return false;
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const &v0 = prev();
			auto const &v1 = corner();
			auto const &v2 = next();
			auto const v0v1 = Segment(v0, v1);
			auto const v1v2 = Segment(v1, v2);
			auto const v2v0 = Segment(v2, v0);
			auto search = rtree.search(bounds);
			return std::none_of(search.begin(), search.end(), [&](auto const &other) {
				if (other == corner) return false;
				auto const &[u0, u1, u2] = other;
				auto const u0u1 = Segment(u0, u1);
				auto const u1u2 = Segment(u1, u2);
				if (v0 == u1) return v0v1 >= u0 && v1v2 >= u0 && v2v0 >= u0;
				if (v2 == u1) return v0v1 >= u2 && v1v2 >= u2 && v2v0 >= u2;
				if (v0 == u2) return v2v0 & u0u1;
				if (v2 == u0) return v2v0 & u1u2;
				return v2v0 & u0u1 || v2v0 & u0u1;
			});
		}

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

public:
	void simplify_one_sided(double tolerance, bool erode) {
		auto corners = Corners();
		auto ordered = Ordered();
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto corner: ring.corners())
					corners.push_back(corner);
		auto rtree = RTree(corners, 1);
		for (auto const &corner: corners)
			if (auto const candidate = Candidate(corner); candidate(rtree, tolerance, erode))
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
				if (auto const candidate = Candidate(corner); candidate(rtree, tolerance, erode))
					ordered.insert(candidate);
		}
	}
};

#endif
