#ifndef CORNER_HPP
#define CORNER_HPP

#include "vector.hpp"
#include <utility>
#include <cmath>

using Corner = std::tuple<Vector<2>, Vector<2>, Vector<2>>;

auto operator<(const Corner &corner1, const Corner &corner2) {
	auto &[v00, v01, v02] = corner1;
	auto &[v10, v11, v12] = corner2;
	return std::abs((v01 - v00) ^ (v02 - v01)) < std::abs((v11 - v10) ^ (v12 - v11));
}

#endif
