#ifndef LAS_HPP
#define LAS_HPP

#include "raw_point.hpp"
#include <istream>
#include <cstddef>
#include <stdexcept>

class LAS {
	std::istream &input;

public:
	std::size_t count;

	LAS(std::istream &input) : input(input), count(0) {
		throw std::runtime_error("LAS reader not implemented");
	}

	auto point() {
		RawPoint point;
		return point;
	}
};

#endif
