////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef BOUNDS_HPP
#define BOUNDS_HPP

#include "point.hpp"
#include <limits>
#include <algorithm>
#include <utility>

struct Bounds {
	double xmin, ymin, xmax, ymax;

	Bounds(double xmin, double ymin, double xmax, double ymax) : xmin(xmin), ymin(ymin), xmax(xmax), ymax(ymax) { }

	Bounds() : Bounds(
		std::numeric_limits<double>::infinity(),
		std::numeric_limits<double>::infinity(),
		-std::numeric_limits<double>::infinity(),
		-std::numeric_limits<double>::infinity()
	) { }

	auto &operator+=(Point const &point) {
		auto const &[x, y] = point;
		xmin = std::min(xmin, x), xmax = std::max(xmax, x);
		ymin = std::min(ymin, y), ymax = std::max(ymax, y);
		return *this;
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
