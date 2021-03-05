#ifndef RAW_POINT_HPP
#define RAW_POINT_HPP

struct RawPoint {
	double x, y, z;
	unsigned char c;

	auto ground() const {
		return 2 == c || 3 == c;
	}
};

#endif
