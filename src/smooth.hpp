////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SMOOTH_HPP
#define SMOOTH_HPP

#include "corner.hpp"
#include "ring.hpp"
#include "vector.hpp"
#include "bounds.hpp"
#include "rtree.hpp"
#include <algorithm>
#include <set>
#include <vector>

template <typename Polygons>
class Smooth {
	using Corner = ::Corner<Ring>;
	using RTree = ::RTree<Corner>;
	using Vertex = Vector<2>;

	struct Candidate {
		Corner corner;
		Bounds bounds;
		Vertex vertex;
		double square_curvature_delta;

		Candidate(Corner const &corner) :
			corner(corner),
			bounds(corner)
		{
			auto const &[v0, v1, v2] = corner.prev();
			auto const &[ _, v3, v4] = corner.next();
			vertex = (v1 + v2 + v3) / 3.0;

			auto const n01 = (v1 - v0).normalise();
			auto const n12 = (v2 - v1).normalise();
			auto const n1v = (vertex - v1).normalise();
			auto const nv3 = (v3 - vertex).normalise();
			auto const n23 = (v3 - v2).normalise();
			auto const n34 = (v4 - v3).normalise();
			square_curvature_delta = n01 * n12 + n12 * n23 + n23 * n34 - n01 * n1v - n1v * nv3 - nv3 * n34;
		}

		friend auto operator==(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.corner == candidate2.corner;
		}

		friend auto operator<(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.square_curvature_delta < candidate2.square_curvature_delta;
		}

		auto operator()(RTree const &rtree) const {
			if (square_curvature_delta >= 0)
				return false;
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const &v0 = prev();
			auto const &v1 = vertex;
			auto const &v2 = next();
			auto search = rtree.search(bounds);
			return std::none_of(search.begin(), search.end(), [&](auto const &other) {
				if (other == corner || other == prev || other == next)
					return false;
				auto const &[u0, u1, u2] = other;
				auto const v01 = v1 - v0;
				auto const v12 = v2 - v1;
				auto const u01 = u1 - u0;
				auto const u12 = u2 - u1;
				// TODO: check for intersection of colinear segments
				if (                        (v01 ^ (u0 - v0)) <=> 0 != (v01 ^ (u1 - v0)) <=> 0 && (u01 ^ (v0 - u0)) <=> 0 != (u01 ^ (v1 - u0)) <=> 0)
					return true;
				if (other.next() != prev && (v01 ^ (u1 - v0)) <=> 0 != (v01 ^ (u2 - v0)) <=> 0 && (u12 ^ (v0 - u1)) <=> 0 != (u12 ^ (v1 - u1)) <=> 0)
					return true;
				if (other.prev() != next && (v12 ^ (u0 - v1)) <=> 0 != (v12 ^ (u1 - v1)) <=> 0 && (u01 ^ (v1 - u0)) <=> 0 != (u01 ^ (v2 - u0)) <=> 0)
					return true;
				if (                        (v12 ^ (u1 - v1)) <=> 0 != (v12 ^ (u2 - v1)) <=> 0 && (u12 ^ (v1 - u1)) <=> 0 != (u12 ^ (v2 - u1)) <=> 0)
					return true;
				return false;
			});
		}

		void replace(RTree &rtree) const {
			auto const next = corner.next();
			auto const prev = corner.prev();
			auto const next_bounds = Bounds(next);
			auto const prev_bounds = Bounds(prev);
			auto const new_corner = corner.replace(vertex);
			rtree.update(corner, bounds, new_corner);
			rtree.update(next, next_bounds);
			rtree.update(prev, prev_bounds);
		}
	};

	using Ordered = std::multiset<Candidate>;
	using Corners = std::vector<Corner>;

public:
	void smooth(int threads) {
		auto corners = Corners();
		auto ordered = Ordered();
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto corner: ring.corners())
					corners.push_back(corner);
		auto rtree = RTree(corners, threads);
		for (auto const &corner: corners)
			if (auto const candidate = Candidate(corner); candidate(rtree))
				ordered.insert(candidate);
		while (!ordered.empty()) {
			auto const least = ordered.begin();
			auto const candidate = *least;
			ordered.erase(least);
			auto updates = Corners();
			for (auto const &corner: rtree.search(candidate.bounds)) {
				auto const candidate = Candidate(corner);
				auto const [begin, end] = ordered.equal_range(candidate);
				auto const position = std::find(begin, end, candidate);
				if (position != end) {
					ordered.erase(position);
					updates.push_back(corner);
				}
			};
			candidate.replace(rtree);
			for (auto const &corner: updates)
				if (auto const candidate = Candidate(corner); candidate(rtree))
					ordered.insert(candidate);
		}
	}
};

#endif
