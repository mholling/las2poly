#ifndef THINNED_HPP
#define THINNED_HPP

#include <unordered_set>
#include <vector>
#include <utility>

template <typename T>
class Thinned {
	std::unordered_set<T, typename T::Hash> thinned;

public:
	template <typename F>
	auto &insert(const T &element, F better_than) {
		auto [existing, inserted] = thinned.insert(element);
		if (!inserted && better_than(element, *existing)) {
			thinned.erase(*existing);
			thinned.insert(element);
		}
		return *this;
	}

	std::vector<T> to_vector() {
		std::vector<T> result;
		result.reserve(thinned.size());
		for (auto element = thinned.begin(); element != thinned.end(); )
			result.push_back(std::move(thinned.extract(element++).value()));
		return result;
	}
};

#endif
