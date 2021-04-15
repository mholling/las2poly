////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distrubuted under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef IEEE754_HPP
#define IEEE754_HPP

namespace IEEE754 {
	unsigned consteval bits(double epsilon = 1.0, unsigned result = 0) {
		return 1.0 + epsilon == 1.0 ? result : bits(epsilon * 0.5, ++result);
	}

	double consteval epsilon(double result = 1.0) {
		return 1.0 + result == 1.0 ? result : epsilon(result * 0.5);
	}
};

#endif

