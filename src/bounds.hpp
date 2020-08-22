#ifndef BOUNDS_HPP
#define BOUNDS_HPP

#include <array>
#include <cstddef>
#include <algorithm>

struct Bounds {
	std::array<std::array<double, 2>, 2> bounds;

	auto operator[](std::size_t pos) const { return bounds[pos]; }

	friend auto operator+(const Bounds &bounds1, const Bounds &bounds2) {
		return Bounds({
			std::min(bounds1[0][0], bounds2[0][0]), std::max(bounds1[0][1], bounds2[0][1]),
			std::min(bounds1[1][0], bounds2[1][0]), std::max(bounds1[1][1], bounds2[1][1])
		});
	}

	friend auto operator&&(const Bounds &bounds1, const Bounds &bounds2) {
		return (bounds1[0][0] < bounds2[0][1]) && (bounds1[0][1] > bounds2[0][0]) && (bounds1[1][0] < bounds2[1][1]) && (bounds1[1][1] > bounds2[1][0]);
	}
};

#endif
