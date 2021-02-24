#ifndef POINT_HPP
#define POINT_HPP

#include "vector.hpp"
#include "bounds.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <cmath>
#include <ostream>

class Point : public Vector<3> {
	unsigned char c;
	std::array<std::uint32_t, 2> indices;

public:
	struct Hash {
		std::size_t operator()(const Point &point) const {
			return static_cast<std::size_t>(point.indices[0]) | static_cast<std::size_t>(point.indices[1]) << 32;
		}
	};

	Point() { }

	Point(std::ifstream &input, double cell_size) {
		for (auto &value: values)
			input.read(reinterpret_cast<char *>(&value), sizeof(value));
		input.read(reinterpret_cast<char *>(&c), sizeof(c));
		indices = {
			static_cast<std::uint32_t>(std::floor(values[0] / cell_size)),
			static_cast<std::uint32_t>(std::floor(values[1] / cell_size))
		};
	}

	auto bounds() const {
		return Bounds({{values[0], values[0]}, {values[1], values[1]}});
	}

	auto is_ground() const {
		return 2 == c || 3 == c;
	}

	friend auto operator==(const Point &point1, const Point &point2) {
		return point1.indices == point2.indices;
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
