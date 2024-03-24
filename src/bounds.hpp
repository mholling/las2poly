////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef BOUNDS_HPP
#define BOUNDS_HPP

#include <limits>
#include <vector>
#include <tuple>
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
			bounds1.xmin >= bounds2.xmin && bounds1.xmax <= bounds2.xmax &&
			bounds1.ymin >= bounds2.ymin && bounds1.ymax <= bounds2.ymax;
	}

	friend auto operator&(Bounds const &bounds1, Bounds const &bounds2) {
		return
			bounds1.xmax >= bounds2.xmin && bounds1.xmin <= bounds2.xmax &&
			bounds1.ymax >= bounds2.ymin && bounds1.ymin <= bounds2.ymax;
	}

	auto empty() const {
		return xmin > xmax;
	}

	template <typename Elements>
	Bounds(Elements const &elements) : Bounds() {
		for (auto const &element: elements)
			*this += Bounds(element);
	}
};

#endif
