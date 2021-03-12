#ifndef POINT_HPP
#define POINT_HPP

#include "record.hpp"
#include "vector.hpp"
#include <cstdint>
#include <functional>
#include <cstddef>

struct Point : Vector<3> {
	bool ground;
	std::uint32_t index;

	Point(const Record &record, std::uint32_t index) : Vector<3>({record.x, record.y, record.z}), ground(2 == record.c || 3 == record.c), index(index) { }

	friend auto operator==(const Point &point1, const Point &point2) {
		return point1.index == point2.index;
	}

	friend Vector<2> operator-(const Point &point1, const Point &point2) {
		return {point1[0] - point2[0], point1[1] - point2[1]};
	}

	operator Vector<2>() const {
		return {(*this)[0], (*this)[1]};
	}

	auto in_circle(const Point &a, const Point &b, const Point &c) const {
		auto aa = a - *this, bb = b - *this, cc = c - *this;
		return (aa * aa) * (bb ^ cc) + (bb * bb) * (cc ^ aa) + (cc * cc) * (aa ^ bb) > 0;
	}
};

template <> struct std::hash<const Point &> {
	std::size_t operator()(const Point &point) const { return point.index; }
};

#endif
