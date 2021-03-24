#ifndef RECORD_HPP
#define RECORD_HPP

#include <utility>

struct Record {
	double x, y, z;
	unsigned char classification;
	bool key_point, withheld, overlap;
};

auto operator<(const Record &r1, const Record &r2) {
	return
		std::pair(2 == r1.classification ? 0 : 3 == r1.classification ? 1 : 2, r1.z) <
		std::pair(2 == r2.classification ? 0 : 3 == r2.classification ? 1 : 2, r2.z);
}

#endif
