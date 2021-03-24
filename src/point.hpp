#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"
#include "record.hpp"

struct Point : Vector<2> {
	float elevation;
	bool ground;

	Point(const Record &record) : Vector({{record.x, record.y}}), elevation(record.z), ground(2 == record.classification || 3 == record.classification) { }

	const Vector<3> operator+() const {
		return {{(*this)[0], (*this)[1], elevation}};
	}

	auto in_circle(const Point &a, const Point &b, const Point &c) const {
		auto aa = a - *this, bb = b - *this, cc = c - *this;
		return (aa * aa) * (bb ^ cc) + (bb * bb) * (cc ^ aa) + (cc * cc) * (aa ^ bb) > 0;
	}
};

#endif
