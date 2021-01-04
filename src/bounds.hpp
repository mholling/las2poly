#ifndef BOUNDS_HPP
#define BOUNDS_HPP

#include <cstddef>
#include <algorithm>

struct Bounds {
	struct {
		double min, max;
	} x, y;

	friend auto operator+(const Bounds &bounds1, const Bounds &bounds2) {
		return Bounds({
			{std::min(bounds1.x.min, bounds2.x.min), std::max(bounds1.x.max, bounds2.x.max)},
			{std::min(bounds1.y.min, bounds2.y.min), std::max(bounds1.y.max, bounds2.y.max)}
		});
	}

	friend auto operator&&(const Bounds &bounds1, const Bounds &bounds2) {
		return (bounds1.x.min < bounds2.x.max) && (bounds1.x.max > bounds2.x.min) && (bounds1.y.min < bounds2.y.max) && (bounds1.y.max > bounds2.y.min);
	}
};

#endif
