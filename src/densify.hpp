////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef DENSIFY_HPP
#define DENSIFY_HPP

#include "ring.hpp"
#include <cmath>

template <typename Polygons>
struct Densify {
	void densify(double separation) {
		for (auto &polygon: static_cast<Polygons &>(*this))
			for (auto &ring: polygon)
				for (auto const corner: ring.corners()) {
					auto const &[v0, v1, v2] = corner;
					int const N = std::ceil((v1 - v0).norm() / separation);
					for (auto n = 1; n < N; ++n)
						corner.insert((v1 * n + v0 * (N - n)) / N);
				}
	}
};

#endif
