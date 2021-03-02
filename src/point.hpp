#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"
#include <cstddef>
#include <functional>
#include <ostream>

struct Point : Vector<3> {
	bool ground;
	std::size_t index;

	Point(double x, double y, double z, bool ground, std::size_t index) : Vector<3>({x, y, z}), ground(ground), index(index) { }
	Point(Point &&point, std::size_t index) : Vector<3>(point), ground(point.ground), index(index) { }

	friend auto operator==(const Point &point1, const Point &point2) {
		return point1.index == point2.index;
	}

	friend auto operator<(const Point &point1, const Point &point2) {
		return point1[0] < point2[0] ? true : point1[0] > point2[0] ? false : point1[1] < point2[1];
	}

	friend Vector<2> operator-(const Point &point1, const Point &point2) {
		return {point1[0] - point2[0], point1[1] - point2[1]};
	}

	friend std::ostream &operator<<(std::ostream &json, const Point &point) {
		return json << '[' << point[0] << ',' << point[1] << ']';
	}

	auto in_circle(const Point &a, const Point &b, const Point &c) const {
		auto aa = a - *this, bb = b - *this, cc = c - *this;
		return (aa * aa) * (bb ^ cc) + (bb * bb) * (cc ^ aa) + (cc * cc) * (aa ^ bb) > 0;
	}
};

template <> struct std::hash<Point> {
	std::size_t operator()(const Point &point) const { return point.index; }
};

#endif
