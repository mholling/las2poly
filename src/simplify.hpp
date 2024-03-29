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
		double area;
		bool removable;

		Candidate(Corner const &corner, double scale, bool erode, bool area_only) :
			corner(corner),
			bounds(corner)
		{
			auto const &[v0, v1, v2] = corner;
			auto cross = (v1 - v0) ^ (v2 - v1);
			auto length = (v2 - v0).norm();
			area = 0.5 * std::abs(cross);
			removable =
				erode == (cross > 0) &&
				area < scale * scale &&
				(area_only || length < 2 * scale);
		}

		friend auto operator==(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.corner == candidate2.corner;
		}

		friend auto operator<(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.area < candidate2.area;
		}

		auto operator()(RTree const &rtree) const {
			if (!removable) return false;
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

	void simplify_one_sided(double scale, bool erode, bool area_only) {
		auto corners = Corners();
		auto ordered = Ordered();
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto corner: ring.corners())
					corners.push_back(corner);
		auto rtree = RTree(corners, 1);
		for (auto const &corner: corners)
			if (auto const candidate = Candidate(corner, scale, erode, area_only); candidate(rtree))
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
				auto const candidate = Candidate(corner, scale, erode, area_only);
				auto const [begin, end] = ordered.equal_range(candidate);
				auto const position = std::find(begin, end, candidate);
				if (position != end)
					ordered.erase(position);
			}
			candidate.erase(rtree);
			for (auto const &corner: updates)
				if (auto const candidate = Candidate(corner, scale, erode, area_only); candidate(rtree))
					ordered.insert(candidate);
		}
	}

public:
	void simplify(double scale, bool erode_then_dilate, bool area_only = true) {
		simplify_one_sided(scale, erode_then_dilate, area_only);
		simplify_one_sided(scale, !erode_then_dilate, area_only);
	}
};

#endif
