#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"
#include <tuple>

struct Point : Vector<2> {
	float elevation;
	unsigned char classification;
	bool key_point, withheld, overlap;

	Point(double x, double y, double z, unsigned char classification, bool key_point, bool withheld, bool overlap) :
		Vector{{x, y}},
		elevation(z),
		classification(classification),
		key_point(key_point),
		withheld(withheld),
		overlap(overlap)
	{ }

	auto ground() const {
		return 2 == classification || 3 == classification;
	}

	Vector<3> const operator+() const {
		return {{(*this)[0], (*this)[1], elevation}};
	}
};

auto operator>(Point const &p1, Point const &p2) {
	return
		std::tuple(p1.key_point, 2 == p1.classification ? 2 : 8 == p1.classification ? 2 : 3 == p1.classification ? 1 : 0, p2.elevation) <
		std::tuple(p2.key_point, 2 == p2.classification ? 2 : 8 == p2.classification ? 2 : 3 == p2.classification ? 1 : 0, p1.elevation);
}

template <>
struct std::tuple_size<Point> : std::integral_constant<std::size_t, 2> { };

template <std::size_t M>
struct std::tuple_element<M, Point> { using type = double; };

#endif
