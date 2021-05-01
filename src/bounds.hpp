////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef BOUNDS_HPP
#define BOUNDS_HPP

#include <limits>
#include <algorithm>
#include <utility>

struct Bounds {
	double xmin, ymin, xmax, ymax;

	Bounds() :
		xmin(std::numeric_limits<double>::infinity()),
		ymin(std::numeric_limits<double>::infinity()),
		xmax(-std::numeric_limits<double>::infinity()),
		ymax(-std::numeric_limits<double>::infinity())
	{ }

	Bounds(Bounds const &bounds) = default;

	template <typename Point>
	Bounds(Point const &point) {
		auto const &[x, y] = point;
		xmin = xmax = x, ymin = ymax = y;
	}

	template <typename Point, typename ...Points>
	Bounds(Point const &point, Points const &...points) : Bounds(points...) {
		auto const &[x, y] = point;
		xmin = std::min(xmin, x), xmax = std::max(xmax, x);
		ymin = std::min(ymin, y), ymax = std::max(ymax, y);
	}

	auto &operator+=(Bounds const &other) {
		xmin = std::min(xmin, other.xmin), xmax = std::max(xmax, other.xmax);
		ymin = std::min(ymin, other.ymin), ymax = std::max(ymax, other.ymax);
		return *this;
	}

	friend auto operator+(Bounds const &bounds1, Bounds const &bounds2) {
		return Bounds(bounds1) += bounds2;
	}

	friend auto operator<=(Bounds const &bounds1, Bounds const &bounds2) {
		return
			bounds1.xmin >= bounds2.xmin && bounds1.xmax <= bounds1.xmax &&
			bounds1.ymin >= bounds2.ymin && bounds1.ymax <= bounds1.ymax;
	}

	friend auto operator&&(Bounds const &bounds1, Bounds const &bounds2) {
		return
			bounds1.xmax >= bounds2.xmin && bounds1.xmin <= bounds2.xmax &&
			bounds1.ymax >= bounds2.ymin && bounds1.ymin <= bounds2.ymax;
	}

	auto empty() const {
		return xmin > xmax;
	}

	struct CompareXMin {
		auto operator()(Bounds const &bounds1, Bounds const &bounds2) const {
			return bounds1.xmin < bounds2.xmin;
		}
	};

	struct CompareXMax {
		auto operator()(Bounds const &bounds1, Bounds const &bounds2) const {
			return bounds1.xmax < bounds2.xmax;
		}
	};

	struct CompareYMin {
		auto operator()(Bounds const &bounds1, Bounds const &bounds2) const {
			return bounds1.ymin < bounds2.ymin;
		}
	};

	struct CompareYMax {
		auto operator()(Bounds const &bounds1, Bounds const &bounds2) const {
			return bounds1.ymax < bounds2.ymax;
		}
	};
};

#endif
