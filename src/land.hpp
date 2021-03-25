#ifndef LAND_HPP
#define LAND_HPP

#include "polygon.hpp"
#include "mesh.hpp"
#include "triangles.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include "ring.hpp"
#include <vector>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <ostream>
#include <utility>

struct Land : std::vector<Polygon> {
	static auto is_water(const Triangles &triangles, double delta, double slope) {
		Vector<3> perp = {{0, 0, 0}};
		long double sum_abs = 0;
		std::size_t count = 0;

		for (auto edges: triangles) {
			std::rotate(edges.begin(), std::min_element(edges.begin(), edges.end()), edges.end());
			perp += edges[1] % edges[2];
			for (auto edge = edges.begin() + 1; edge != edges.end(); ++edge)
				if (edge->first->ground && edge->second->ground)
					++count, sum_abs += std::abs(edge->second->elevation - edge->first->elevation);
		}

		auto angle = std::acos(std::abs(perp[2] / perp.norm()));
		return angle < slope && count > 0 && sum_abs < delta * count;
	}

	Land(Mesh &mesh, double length, double width, double slope, double area) {
		Triangles large_triangles;
		Edges outside_edges;
		auto delta = width * std::tan(slope);

		mesh.deconstruct([&, length](const auto &triangle) {
			if (triangle > length)
				large_triangles += triangle;
		}, [&](const auto &edge) {
			outside_edges.insert(-edge);
		});

		large_triangles.explode([=, &outside_edges](const auto &&triangles) {
			if ((outside_edges || triangles) || ((width <= length || triangles > width) && is_water(triangles, delta, slope)))
				for (const auto &triangle: triangles)
					outside_edges -= triangle;
		});

		auto rings = Rings(outside_edges);
		auto rings_end = std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		});
		auto holes_begin = std::partition(rings.begin(), rings_end, [](const auto &ring) {
			return ring > 0;
		});
		std::sort(rings.begin(), holes_begin);

		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&](const auto &exterior) {
			Polygon polygon = {exterior};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings_end, [&](const auto &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(polygon));
			emplace_back(polygon);
		});
	}

	void simplify(double tolerance) {
		for (auto &polygon: *this)
			for (auto &ring: polygon)
				ring.simplify(tolerance);
	}

	void smooth(double tolerance, double angle) {
		for (auto &polygon: *this)
			for (auto &ring: polygon)
				ring.smooth(tolerance, angle);
	}
};

auto &operator<<(std::ostream &json, const Land &land) {
	auto separator = '[';
	for (const auto &polygon: land)
		json << std::exchange(separator, ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
	return json << (separator == '[' ? "[]" : "]");
}

#endif
