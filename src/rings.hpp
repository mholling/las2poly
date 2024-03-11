////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef RINGS_HPP
#define RINGS_HPP

#include "ring.hpp"
#include "vertex.hpp"
#include "link.hpp"
#include "edges.hpp"
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <algorithm>
#include <utility>

class Rings : public std::vector<Ring> {
	using VerticesLinks = std::unordered_multimap<Vertex, Link>;
	using Connections = std::unordered_map<Link, Link>;

	void load(Links const &links, bool allow_self_intersection, bool exterior) {
		auto vertices_links = VerticesLinks();
		for (auto const &link: links)
			vertices_links.emplace(link.first, link);

		auto connections = Connections();
		for (auto const &incoming: links) {
			auto const ordering = [&](auto const &vertex_link1, auto const &vertex_link2) {
				auto const &v1 = vertex_link1.second.second;
				auto const &v2 = vertex_link2.second.second;
				return incoming < v1
					? incoming > v2 || Link(v1, v2) > incoming.second
					: incoming > v2 && Link(v1, v2) > incoming.second;
			};
			auto const [start, stop] = vertices_links.equal_range(incoming.second);
			auto const &outgoing = allow_self_intersection
				? std::min_element(start, stop, ordering)->second
				: std::max_element(start, stop, ordering)->second;
			connections.emplace(incoming, outgoing);
		}

		auto interior_links = Links();
		while (!connections.empty()) {
			auto links = Links();
			for (auto connection = connections.begin(); connection != connections.end(); ) {
				links.push_back(connection->first);
				connections.erase(std::exchange(connection, connections.find(connection->second)));
			}

			auto const ring = Ring(links);
			if (!exterior)
				push_back(ring);
			else if (ring.exterior())
				push_back(ring);
			else
				interior_links.insert(interior_links.end(), links.begin(), links.end());
		}

		if (exterior) {
			auto const holes = Rings(interior_links, allow_self_intersection, false);
			insert(end(), holes.begin(), holes.end());
		}
	}

public:
	Rings(Links const &links, bool allow_self_intersection, bool exterior = true) {
		load(links, allow_self_intersection, exterior);
	}

	Rings(Edges const &edges, bool allow_self_intersection) {
		auto links = Links();
		for (auto const &[p1, p2]: edges)
			links.emplace_back(*p1, *p2);
		load(links, allow_self_intersection, true);
	}
};

#endif
