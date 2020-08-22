#ifndef PLANE_HPP
#define PLANE_HPP

#include "vector.hpp"
#include "matrix.hpp"
#include "point.hpp"
#include <cmath>

class Plane {
	Vector<3> centroid, normal;

public:
	Plane() { }

	template <typename C>
	Plane(const C &points) {
		auto [mean, covariance] = Matrix<3>::statistics(points);
		for (int n0 = 0, n1 = 1, n2 = 2; n0 < 3; ++n0, ++n1 %= 3, ++n2 %= 3) {
			auto cross = covariance[n1] ^ covariance[n2];
			auto det = cross[n0];
			auto weight = ((normal * cross) < 0 ? -det : det) * det;
			normal += cross * weight;
		}
		normal.normalise();
		centroid = mean;
	}

	auto error(const Point &point) const {
		return (centroid - point) * normal;
	}

	auto slope() const {
		return std::acos(std::abs(normal[2])) * 180.0 / M_PI;
	}
};

#endif
