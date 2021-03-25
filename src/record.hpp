#ifndef RECORD_HPP
#define RECORD_HPP

#include <utility>

struct Record {
	double x, y, z;
	unsigned char classification;
	bool key_point, withheld, overlap;
};

auto operator>(const Record &r1, const Record &r2) {
	return
		std::tuple(r1.key_point, 2 == r1.classification ? 2 : 8 == r1.classification ? 2 : 3 == r1.classification ? 1 : 0, r2.z) >
		std::tuple(r2.key_point, 2 == r2.classification ? 2 : 8 == r2.classification ? 2 : 3 == r2.classification ? 1 : 0, r1.z);
}

#endif
