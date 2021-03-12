#ifndef RECORD_HPP
#define RECORD_HPP

#include <utility>

struct Record {
	double x, y, z;
	unsigned char c;
};

auto operator<(const Record &r1, const Record &r2) {
	return
		std::pair(2 == r1.c ? 0 : 3 == r1.c ? 1 : 2, r1.z) <
		std::pair(2 == r2.c ? 0 : 3 == r2.c ? 1 : 2, r2.z);
}

#endif
