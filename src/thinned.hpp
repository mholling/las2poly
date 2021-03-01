#ifndef THINNED_HPP
#define THINNED_HPP

#include "thinned.hpp"
#include <unordered_set>
#include <vector>
#include <utility>

class Thinned {
	std::unordered_set<Point> thinned;

public:
	template <typename Function>
	auto &insert(const Point &point, Function better_than) {
		auto [existing, inserted] = thinned.insert(point);
		if (!inserted && better_than(point, *existing)) {
			thinned.erase(*existing);
			thinned.insert(point);
		}
		return *this;
	}

	std::vector<Point> to_vector() {
		std::vector<Point> result;
		result.reserve(thinned.size());
		std::size_t index = 0;
		for (auto point = thinned.begin(); point != thinned.end(); ) 
			result.push_back(Point(std::move(thinned.extract(point++).value()), index++));
		return result;
	}
};

#endif
