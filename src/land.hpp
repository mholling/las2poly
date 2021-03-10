#ifndef LAND_HPP
#define LAND_HPP

#include "polygon.hpp"
#include "mesh.hpp"
#include "triangles.hpp"
#include "edges.hpp"
#include "rings.hpp"
#include "ring.hpp"
#include <algorithm>
#include <vector>
#include <iterator>
#include <ostream>

class Land : std::vector<Polygon> {
	static auto is_water(const Triangles &triangles, double delta, double slope, bool permissive) {
		Vector<3> perp;
		double sum_abs = 0.0;
		std::size_t count = 0;

		for (const auto &triangle: triangles) {
			auto edge = triangle.begin();
			std::array edges = {edge, ++edge, ++edge};
			std::iter_swap(edges.begin(), std::min_element(edges.begin(), edges.end(), [](const auto &edge1, const auto *edge2) {
				return *edge1 < *edge2;
			}));
			perp += *edges[1] % *edges[2];
			for (const auto &edge: {edges[1], edges[2]})
				if (edge->first.ground && edge->second.ground) {
					sum_abs += std::abs(edge->second[2] - edge->first[2]);
					++count;
				}
		}

		auto angle = std::acos(std::abs(perp.normalise()[2])) * 180.0 / M_PI;
		return angle > slope ? false : count < 3 ? permissive : sum_abs / count < delta;
	}

public:
	Land(Mesh &mesh, double length, double width, double delta, double slope, double area, bool permissive) {
		Triangles large_triangles;
		Edges outside_edges;

		mesh.deconstruct([&](const auto &triangle) {
			if (triangle > length)
				large_triangles += triangle;
		}, [&](const auto &edge) {
			outside_edges.insert(edge);
		});

		large_triangles.explode([&](const auto &triangles) {
			if ((outside_edges || triangles) || ((width <= length || triangles > width) && is_water(triangles, delta, slope, permissive)))
				for (const auto &triangle: triangles)
					outside_edges -= triangle;
		});

		auto rings = Rings(outside_edges)();
		auto rings_end = std::remove_if(rings.begin(), rings.end(), [=](const auto &ring) {
			return ring < area && ring > -area;
		});
		auto holes_begin = std::partition(rings.begin(), rings_end, [](const auto &ring) {
			return ring > 0;
		});
		std::sort(rings.begin(), holes_begin);

		auto remaining = holes_begin;
		std::for_each(rings.begin(), holes_begin, [&](const auto &exterior) {
			std::vector<Ring> rings = {exterior};
			auto old_remaining = remaining;
			remaining = std::partition(remaining, rings_end, [&](const auto &hole) {
				return exterior.contains(hole);
			});
			std::copy(old_remaining, remaining, std::back_inserter(rings));
			emplace_back(rings);
		});
	}

	friend auto &operator<<(std::ostream &json, const Land &land) {
		bool first = true;
		for (const auto &polygon: land)
			json << (std::exchange(first, false) ? '[' : ',') << "{\"type\":\"Feature\",\"properties\":null,\"geometry\":{\"type\":\"Polygon\",\"coordinates\":" << polygon << "}}";
		json << (first ? "[]" : "]");
		return json;
	}
};

#endif
