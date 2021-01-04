#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"
#include "bounds.hpp"
#include <cstddef>
#include <fstream>
#include <cmath>
#include <ostream>

class Point : public Vector<3> {
	unsigned char c;
	std::size_t index;

public:
	struct Hash {
		std::size_t operator()(const Point &point) const { return point.index; }
	};

	Point() { }

	Point(std::ifstream &input, std::size_t index) : index(index) {
		for (auto &value: *this)
			input.read(reinterpret_cast<char *>(&value), sizeof(value));
		input.read(reinterpret_cast<char *>(&c), sizeof(c));
	}

	auto bounds() const {
		return Bounds({{(*this)[0], (*this)[0]}, {(*this)[1], (*this)[1]}});
	}

	auto vegetation(unsigned char max) const {
		return 2 <= c && c <= max;
	}

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
};

#endif
