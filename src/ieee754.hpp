#ifndef IEEE754_HPP
#define IEEE754_HPP

struct IEEE754 {
	static constexpr unsigned bits(double epsilon = 1.0, unsigned result = 0) {
		return 1.0 + epsilon == 1.0 ? result : bits(epsilon * 0.5, ++result);
	}

	static constexpr double epsilon(double result = 1.0) {
		return 1.0 + result == 1.0 ? result : epsilon(result * 0.5);
	}
};

#endif

