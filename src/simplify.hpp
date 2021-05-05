////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SIMPLIFY_HPP
#define SIMPLIFY_HPP

#include "vector.hpp"
#include "segment.hpp"
#include "ring.hpp"
#include "bounds.hpp"
#include "polygons.hpp"
#include "rtree.hpp"
#include <tuple>
#include <utility>
#include <cmath>
#include <algorithm>
#include <set>
#include <vector>

class Simplify {
	using Vertex = Vector<2>;
	using Vertices = std::tuple<Vertex, Vertex, Vertex>;

	static auto encloses(Vertices const &vertices, Vertex const &vertex) {
		auto const &[v0, v1, v2] = vertices;
		auto const orient0 = Segment(v0, v1) <= vertex;
		auto const orient1 = Segment(v1, v2) <= vertex;
		auto const orient2 = Segment(v2, v0) <= vertex;
		return orient0 == orient1 && orient1 == orient2;
	}

	template <typename ...Others>
	static auto encloses(Vertices const &vertices, Vertex const &vertex, Others const &...others) {
		return encloses(vertices, vertex) || encloses(vertices, others...);
	}

	template <bool erode>
	class OneSided {
		using Corner = Ring::CornerIterator;
		using Ordinal = std::pair<bool, double>;

		struct Candidate : Ordinal {
			Corner corner;
			Bounds bounds;

			static auto ordinal(Corner const &corner, bool withhold) {
				auto const cross = corner.cross();
				return Ordinal(withhold, std::abs(cross));
			}

			template <typename RTree>
			static auto ordinal(Corner const &corner, RTree const &rtree) {
				auto const cross = corner.cross();
				if (erode == (cross < 0) || corner.ring_size() <= 4)
					return Ordinal(true, std::abs(cross));
				auto const prev1 = corner.prev();
				auto const next1 = corner.next();
				auto const prev2 = prev1.prev();
				auto const next2 = next1.next();
				auto const vertices = *corner;
				auto search = rtree.search(corner.bounds());
				auto const withhold = std::any_of(search.begin(), search.end(), [&](auto const &other) {
					if (other == corner)
						return false;
					auto const [u0, u1, u2] = *other;
					if (other == prev1)
						return encloses(vertices, u0);
					if (other == next1)
						return encloses(vertices, u2);
					if (other == prev2 && other == next2)
						return encloses(vertices, u1);
					if (other == prev2)
						return encloses(vertices, u0, u1);
					if (other == next2)
						return encloses(vertices, u1, u2);
					else
						return encloses(vertices, u0, u1, u2);
				});
				return Ordinal(withhold, std::abs(cross));
			}

			template <typename Arg>
			Candidate(Corner const &corner, Arg const &arg) :
				Ordinal(ordinal(corner, arg)),
				corner(corner),
				bounds(corner.bounds())
			{ }

			friend bool operator==(Candidate const &candidate1, Candidate const &candidate2) {
				return candidate1.corner == candidate2.corner;
			}

			friend bool operator<(Candidate const &candidate, double corner_area) {
				return candidate < Ordinal(false, corner_area * 2);
			}
		};

		using Ordered = std::multiset<Candidate>;
		using Corners = std::vector<Corner>;

	public:
		void operator()(Polygons &polygons, double tolerance) {
			auto corners = Corners();
			auto ordered = Ordered();
			for (auto &polygon: polygons)
				for (auto &ring: polygon)
					for (auto corner = ring.begin(); corner != ring.end(); ++corner)
						corners.push_back(corner);
			auto rtree = RTree(corners);
			for (auto const &corner: corners)
				ordered.emplace(corner, rtree);
			for (auto least = ordered.begin(); !ordered.empty() && *least < tolerance; least = ordered.begin()) {
				auto const corner = least->corner;
				auto const bounds = least->bounds;
				ordered.erase(least);
				rtree.erase(corner);
				auto search = rtree.search(bounds);
				auto const neighbours = Corners(search.begin(), search.end());
				for (auto const &neighbour: neighbours)
					for (auto const withhold: {true, false}) {
						auto const candidate = Candidate(neighbour, withhold);
						auto const [begin, end] = ordered.equal_range(candidate);
						auto const position = std::find(begin, end, candidate);
						if (position != end)
							ordered.erase(position);
					}
				auto const next = corner.next();
				auto const prev = corner.prev();
				auto const next_bounds = next.bounds();
				auto const prev_bounds = prev.bounds();
				corner.erase();
				rtree.update(next, next_bounds);
				rtree.update(prev, prev_bounds);
				for (auto const &neighbour: neighbours)
					ordered.emplace(neighbour, rtree);
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
