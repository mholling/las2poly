////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SMOOTH_HPP
#define SMOOTH_HPP

#include "ring.hpp"
#include "vector.hpp"
#include "bounds.hpp"
#include "rtree.hpp"
#include <algorithm>
#include <set>
#include <vector>
#include <cmath>

template <typename Polygons>
class Smooth {
	using Corner = Ring::CornerIterator;
	using Vertex = Vector<2>;

	struct Candidate {
		Corner corner;
		Bounds bounds;
		double cosine;
		Vertex v01, v12;

		Candidate(Corner const &corner) :
			corner(corner),
			bounds(corner.bounds()),
			cosine(corner.cosine())
		{ }

		Candidate(Corner const &corner, double tolerance) : Candidate(corner) {
			auto const &[v0, v1, v2] = *corner;
			auto const f0 = std::min(0.25, tolerance / (v1 - v0).norm());
			auto const f2 = std::min(0.25, tolerance / (v2 - v1).norm());
			v01 = v0 * f0 + v1 * (1.0 - f0);
			v12 = v2 * f2 + v1 * (1.0 - f2);
		}

		friend auto operator==(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.corner == candidate2.corner;
		}

		friend auto operator<(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.cosine < candidate2.cosine;
		}

		template <typename RTree>
		auto smoothable(RTree const &rtree, double max_cosine) const {
			if (cosine > max_cosine)
				return false;
			auto const cross = corner.cross();
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const &vertices = *corner;
			auto search = rtree.search(bounds);
			return std::none_of(search.begin(), search.end(), [&](auto const &other) {
				if (other == corner || other == prev || other == next)
					return false;
				auto const &[v0, v1, v2] = vertices;
				auto const &[u0, u1, u2] = *other;
				auto const cross01 = (u1 - v01) ^ (u1 - v1);
				auto const cross12 = (u1 - v1)  ^ (u1 - v12);
				auto const cross20 = (u1 - v12) ^ (u1 - v01);
				if (cross01 < 0 && cross12 < 0 && cross20 < 0)
					return true;
				if (cross01 > 0 && cross12 > 0 && cross20 > 0)
					return true;
				if (u1 == v1)
					for (auto const &u: {u0, u2}) {
						auto const cross01 = (u - v01) ^ (u - v1);
						auto const cross12 = (u - v1)  ^ (u - v12);
						if (cross01 < 0 && cross12 < 0 && cross < 0)
							return true;
						if (cross01 > 0 && cross12 > 0 && cross > 0)
							return true;
					}
				return false;
			});
		}

		template <typename RTree, typename Updates>
		void replace(RTree &rtree, Updates &updates) const {
			auto const next = corner.next();
			auto const prev = corner.prev();
			auto const next_bounds = next.bounds();
			auto const prev_bounds = prev.bounds();
			auto const &[corner1, corner2] = corner.replace(v01, v12);
			rtree.replace(corner, bounds, corner1, corner2);
			rtree.update(next, next_bounds);
			rtree.update(prev, prev_bounds);
			std::erase(updates, corner);
			updates.push_back(corner1);
			updates.push_back(corner2);
		}
	};

	using Ordered = std::multiset<Candidate>;
	using Corners = std::vector<Corner>;

public:
	void smooth(double tolerance, double angle) {
		auto const cosine = std::cos(angle);
		auto corners = Corners();
		auto ordered = Ordered();
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto corner = ring.begin(); corner != ring.end(); ++corner)
					corners.push_back(corner);
		auto rtree = RTree(corners);
		for (auto const &corner: corners)
			if (auto const candidate = Candidate(corner, tolerance); candidate.smoothable(rtree, cosine))
				ordered.insert(candidate);
		while (!ordered.empty()) {
			auto const candidate = *ordered.begin();
			auto search = rtree.search(candidate.bounds);
			auto updates = Corners(search.begin(), search.end());
			for (auto const &corner: updates) {
				auto const candidate = Candidate(corner);
				auto const &[begin, end] = ordered.equal_range(candidate);
				auto const position = std::find(begin, end, candidate);
				if (position != end)
					ordered.erase(position);
			}
			candidate.replace(rtree, updates);
			for (auto const &corner: updates)
				if (auto const candidate = Candidate(corner, tolerance); candidate.smoothable(rtree, cosine))
					ordered.insert(candidate);
		}
	}
};

#endif
