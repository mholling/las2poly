////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RINGS_HPP
#define RINGS_HPP

#include "ring.hpp"
#include "vertex.hpp"
#include "segment.hpp"
#include "edges.hpp"
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <algorithm>
#include <utility>

class Rings : public std::vector<Ring> {
	using VerticesSegments = std::unordered_multimap<Vertex, Segment>;
	using Connections = std::unordered_map<Segment, Segment>;

	void load(Segments const &segments, bool allow_self_intersection, bool exterior) {
		auto vertices_segments = VerticesSegments();
		for (auto const &segment: segments)
			vertices_segments.emplace(segment.first, segment);

		auto connections = Connections();
		for (auto const &incoming: segments) {
			auto const ordering = [&](auto const &vertex_segment1, auto const &vertex_segment2) {
				auto const &v1 = vertex_segment1.second.second;
				auto const &v2 = vertex_segment2.second.second;
				return incoming < v1
					? incoming > v2 || Segment(v1, v2) > incoming.second
					: incoming > v2 && Segment(v1, v2) > incoming.second;
			};
			auto const [start, stop] = vertices_segments.equal_range(incoming.second);
			auto const &outgoing = allow_self_intersection
				? std::min_element(start, stop, ordering)->second
				: std::max_element(start, stop, ordering)->second;
			connections.emplace(incoming, outgoing);
		}

		auto interior_segments = Segments();
		while (!connections.empty()) {
			auto segments = Segments();
			for (auto connection = connections.begin(); connection != connections.end(); ) {
				segments.push_back(connection->first);
				connections.erase(std::exchange(connection, connections.find(connection->second)));
			}

			auto const ring = Ring(segments);
			if (!exterior)
				push_back(ring);
			else if (ring.exterior())
				push_back(ring);
			else
				interior_segments.insert(interior_segments.end(), segments.begin(), segments.end());
		}

		if (exterior) {
			auto const holes = Rings(interior_segments, allow_self_intersection, false);
			insert(end(), holes.begin(), holes.end());
		}
	}

public:
	Rings(Segments const &segments, bool allow_self_intersection, bool exterior = true) {
		load(segments, allow_self_intersection, exterior);
	}

	Rings(Edges const &edges, bool allow_self_intersection) {
		auto segments = Segments();
		for (auto const &[p1, p2]: edges)
			segments.emplace_back(*p1, *p2);
		load(segments, allow_self_intersection, true);
	}
};

#endif
