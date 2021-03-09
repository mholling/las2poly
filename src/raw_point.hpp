#ifndef RAW_POINT_HPP
#define RAW_POINT_HPP

#include <utility>

struct RawPoint {
	double x, y, z;
	unsigned char c;
};

auto operator<(const RawPoint &p1, const RawPoint &p2) {
	return
		std::pair(2 == p1.c ? 0 : 3 == p1.c ? 1 : 2, p1.z) <
		std::pair(2 == p2.c ? 0 : 3 == p2.c ? 1 : 2, p2.z);
}

#endif
