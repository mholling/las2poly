////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SMOOTH_HPP
#define SMOOTH_HPP

#include "simplify.hpp"
#include "corner.hpp"
#include "ring.hpp"
#include "rtree.hpp"
#include "bounds.hpp"
#include "vertex.hpp"
#include "segment.hpp"
#include "summation.hpp"
#include <utility>
#include <algorithm>
#include <set>
#include <vector>

template <typename Polygons>
class Smooth : public SimplifyOneSided<Polygons, false> {
	using Corner = ::Corner<Ring>;
	using RTree = ::RTree<Corner>;

	struct Candidate {
		Corner corner;
		Bounds bounds;
		Vertex vertex;
		double cosine;
		bool increases_rms_curvature;
		double delta_perimeter;

		Candidate(Corner const &corner) :
			corner(corner),
			bounds(corner)
		{
			auto const &[v0, v1, v2] = corner.prev();
			auto const &[ _, v3, v4] = corner.next();
			vertex = (v1 + v2 + v3) / 3.0;

			auto const d01 = v1 - v0;
			auto const d12 = v2 - v1;
			auto const d1v = vertex - v1;
			auto const dv3 = v3 - vertex;
			auto const d23 = v3 - v2;
			auto const d34 = v4 - v3;

			auto const n01 = d01.norm();
			auto const n12 = d12.norm();
			auto const n1v = d1v.norm();
			auto const nv3 = dv3.norm();
			auto const n23 = d23.norm();
			auto const n34 = d34.norm();

			auto const u01 = d01 / n01;
			auto const u12 = d12 / n12;
			auto const u1v = d1v / n1v;
			auto const uv3 = dv3 / nv3;
			auto const u23 = d23 / n23;
			auto const u34 = d34 / n34;

			cosine = u12 * u23;
			delta_perimeter = n1v + nv3 - n12 - n23;
			increases_rms_curvature = u01 * u12 + u12 * u23 + u23 * u34 - u01 * u1v - u1v * uv3 - uv3 * u34 >= 0;
		}

		friend auto operator==(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.corner == candidate2.corner;
		}

		friend auto operator<(Candidate const &candidate1, Candidate const &candidate2) {
			return candidate1.cosine < candidate2.cosine;
		}

		auto operator()(RTree const &rtree) const {
			if (increases_rms_curvature) return false;
			auto const prev = corner.prev();
			auto const next = corner.next();
			auto const &v0 = prev();
			auto const &v1 = vertex;
			auto const &v2 = next();
			auto const v0v1 = Segment(v0, v1);
			auto const v1v2 = Segment(v1, v2);
			auto search = rtree.search(bounds);
			return std::none_of(search.begin(), search.end(), [&](auto const &other) {
				if (other == corner) return false;
				if (other == prev) return false;
				if (other == next) return false;
				auto const &[u0, u1, u2] = other;
				auto const u0u1 = Segment(u0, u1);
				auto const u1u2 = Segment(u1, u2);
				if (                        v0v1 & u0u1) return true;
				if (other.next() != prev && v0v1 & u1u2) return true;
				if (other.prev() != next && v1v2 & u0u1) return true;
				if (                        v1v2 & u1u2) return true;
				return false;
			});
		}

		template <typename Summation>
		void update(RTree &rtree, Summation &perimeter_summation) const {
			auto const next = corner.next();
			auto const prev = corner.prev();
			auto const next_bounds = Bounds(next);
			auto const prev_bounds = Bounds(prev);
			corner.update(vertex);
			rtree.update(corner, bounds);
			rtree.update(next, next_bounds);
			rtree.update(prev, prev_bounds);
			perimeter_summation += delta_perimeter;
		}
	};

	auto static constexpr perimeter_change_threshold = 0.00001;

	using Ordered = std::multiset<Candidate>;
	using Corners = std::vector<Corner>;

public:
	void smooth(double scale, bool erode_then_dilate) {
		this->simplify_one_sided(scale, erode_then_dilate);
		this->simplify_one_sided(scale, !erode_then_dilate);

		auto corners = Corners();
		auto ordered = Ordered();
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto corner: ring.corners())
					corners.push_back(corner);
		auto perimeter = 0.0;
		auto perimeter_summation = Summation(perimeter);
		for (auto const &[v0, v1, v2]: corners)
			perimeter_summation += (v0 - v1).norm();
		auto rtree = RTree(corners, 1);
		for (int iteration = 0; iteration < 100; ++iteration) {
			auto delta_perimeter = 0.0;
			auto delta_summation = Summation(delta_perimeter);
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
				candidate.update(rtree, delta_summation);
				for (auto const &corner: updates)
					if (auto const candidate = Candidate(corner); candidate(rtree))
						ordered.insert(candidate);
			}
			if (delta_perimeter + perimeter_change_threshold * perimeter > 0)
				break;
			perimeter_summation += delta_perimeter;
		}
	}
};

#endif
