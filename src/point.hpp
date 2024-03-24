////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef POINT_HPP
#define POINT_HPP

#include "vertex.hpp"
#include "vector.hpp"
#include "bounds.hpp"
#include <tuple>
#include <type_traits>

struct Point : Vertex {
	float elevation;
	unsigned char classification;
	bool key_point, withheld, overlap;

	Point(double x, double y, double z, unsigned char classification, bool key_point, bool withheld, bool overlap) :
		Vertex{{x, y}},
		elevation(z),
		classification(classification),
		key_point(key_point),
		withheld(withheld),
		overlap(overlap)
	{ }

	Point(double x, double y) :
		Point(x, y, 0.0, 2, false, true, false)
	{ }

	auto ground() const {
		return 2 == classification;
	}

	auto synthetic() const {
		return withheld;
	}

	void ground(float new_elevation) {
		elevation = new_elevation;
		classification = 2;
	}
};

auto operator+(Point const &point) {
	return Vector<3>{{point[0], point[1], point.elevation}};
}

auto operator>(Point const &p1, Point const &p2) {
	return
		std::tuple(p1.key_point, p1.ground(), p2.elevation) >
		std::tuple(p2.key_point, p2.ground(), p1.elevation);
}

template <>
Bounds::Bounds(Point const &point) {
	std::tie(xmin, ymin) = std::tie(xmax, ymax) = point;
}

template <>
struct std::tuple_size<Point> : std::integral_constant<std::size_t, 2> { };

template <std::size_t M>
struct std::tuple_element<M, Point> { using type = double; };

#endif
