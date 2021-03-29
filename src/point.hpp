#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"

struct Point : Vector<2> {
	float elevation;
	unsigned char classification;
	bool key_point, withheld, overlap;

	Point(double x, double y, double z, unsigned char classification, bool key_point, bool withheld, bool overlap) :
		Vector({{x, y}}),
		elevation(z),
		classification(classification),
		key_point(key_point),
		withheld(withheld),
		overlap(overlap)
	{ }

	auto ground() const {
		return 2 == classification || 3 == classification;
	}

	const Vector<3> operator+() const {
		return {{(*this)[0], (*this)[1], elevation}};
	}

	auto in_circle(const Point &a, const Point &b, const Point &c) const {
		auto aa = a - *this, bb = b - *this, cc = c - *this;
		return (aa * aa) * (bb ^ cc) + (bb * bb) * (cc ^ aa) + (cc * cc) * (aa ^ bb) > 0;
	}
};

#endif
