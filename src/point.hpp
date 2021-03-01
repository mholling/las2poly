#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <cmath>
#include <ostream>

class Point : public Vector<3> {
	unsigned char c;
	std::size_t index;
	friend std::hash<Point>;

public:
	Point() { }

	Point(std::ifstream &input, double cell_size) {
		for (auto &value: values)
			input.read(reinterpret_cast<char *>(&value), sizeof(value));
		input.read(reinterpret_cast<char *>(&c), sizeof(c));
		std::uint32_t index_x = std::floor(values[0] / cell_size);
		std::uint32_t index_y = std::floor(values[1] / cell_size);
		index = static_cast<std::size_t>(index_x) | static_cast<std::size_t>(index_y) << 32;
	}

	Point(Point &&point, std::size_t index) : Vector<3>(point), c(point.c), index(index) { }

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

	auto is_ground() const {
		return 2 == c || 3 == c;
	}

	auto in_circle(const Point &a, const Point &b, const Point &c) const {
		auto aa = a - *this, bb = b - *this, cc = c - *this;
		return aa * aa * (bb ^ cc) + bb * bb * (cc ^ aa) + cc * cc * (aa ^ bb) > 0;
	}
};

template <> struct std::hash<Point> {
	std::size_t operator()(const Point &point) const { return point.index; }
};

#endif
