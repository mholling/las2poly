////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distrubuted under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef EXACT_HPP
#define EXACT_HPP

#include "ieee754.hpp"
#include <cstddef>
#include <array>
#include <utility>
#include <type_traits>
#include <algorithm>
#include <compare>

// partial implementation of:
//     Shewchuk, J. 'Adaptive Precision Floating-Point
//     Arithmetic and Fast Robust Geometric Predicates'

template <std::size_t N = 1>
class Exact : std::array<double, N> {
	using Array = std::array<double, N>;

	template <std::size_t M>
	friend class Exact;

	template <std::size_t M>
	auto partition(Exact<M> &e1, Exact<N-M> &e2) const {
		auto here = this->begin();
		for (auto &e: e1)
			e = *here++;
		for (auto &e: e2)
			e = *here++;
	}

	auto static split(double const &a) {
		auto static constexpr s = 1ul + (1ul << (IEEE754::bits() + 1u) / 2u);
		auto const c = s * a;
		auto const aa = c - a;
		auto const h = c - aa;
		return std::pair(a - h, h);
	};

	void static two_product(double &l, double &h) {
		auto const [ll, lh] = split(l);
		auto const [hl, hh] = split(h);
		h = h * l;
		auto const err1 = h - (lh * hh);
		auto const err2 = err1 - (ll * hh);
		auto const err3 = err2 - (lh * hl);
		l = (ll * hl) - err3;
	}

	void static two_sum(double &l, double &h) {
		auto const x = l + h;
		auto const lv = x - h;
		auto const hv = x - lv;
		auto const lr = l - lv;
		auto const hr = h - hv;
		l = lr + hr, h = x;
	};

	void static two_diff(double &l, double &h) {
		auto const x = l - h;
		auto const hv = l - x;
		auto const lv = x + hv;
		auto const hr = h - hv;
		auto const lr = l - lv;
		l = lr - hr, h = x;
	};

	void static fast_two_sum(double &l, double &h) {
		auto const x = l + h;
		auto const lv = x - h;
		auto const lr = l - lv;
		l = lr, h = x;
	}

	Exact() = default;

public:
	template <typename ...Values, typename = std::enable_if_t<sizeof...(Values) == N>>
	Exact(Values ...values) : Array{{values...}} { }

	friend auto operator<=>(Exact const &exact, int const &zero [[maybe_unused]]) {
		auto const nonzero = std::find_if(exact.rbegin(), exact.rend(), [](auto const &value) {
			return value != 0.0;
		});
		return (nonzero == exact.rend() ? 0.0 : *nonzero) <=> 0.0;
	}

	template <std::size_t M>
	auto operator+(Exact<M> const &other) const {
		auto result = Exact<M+N>();
		if constexpr (N == 2 && M == 2) { // EXPANSION-SUM
			auto h = result.begin();
			for (auto const &e: *this)
				*h++ = e;
			for (auto const &f: other)
				*h++ = f;
			for (std::size_t m = 0; m < M; ++m)
				for (std::size_t n = 0; n < N; ++n)
					two_sum(result[m+n], result[m+N]);
		} else { // FAST-EXPANSION-SUM
			auto e = this->begin(), e_end = this->end();
			auto f = other.begin(), f_end = other.end();
			for (auto &h: result)
				h = e == e_end ? *f++ : f == f_end ? *e++ : *e == 0 ? *e++ : *f == 0 ? *f++ : std::abs(*e) < std::abs(*f) ? *e++ : *f++;
			fast_two_sum(result[0], result[1]);
			for (std::size_t n = 1; n+1 < M + N; ++n) {
				// fast_two_sum(result[n-1], result[n+1]); // add for LINEAR-EXPANSION-SUM
				two_sum(result[n], result[n+1]);
			}
		}
		return result;
	}

	template <std::size_t M>
	auto operator-(Exact<M> const &other) const { // EXPANSION-DIFF
		auto result = Exact<M+N>();
		auto h = result.begin();
		for (auto const &e: *this)
			*h++ = e;
		for (auto const &f: other)
			*h++ = f;
		for (std::size_t m = 0; m < M; ++m) {
			two_diff(result[m], result[m+N]);
			for (std::size_t n = 1; n < N; ++n)
				two_sum(result[m+n], result[m+N]);
		}
		return result;
	}

	template <std::size_t M>
	auto operator*(Exact<M> const &other) const {
		if constexpr (N == 1 && M == 1) { // TWO-PRODUCT
			auto result = Exact<M+N>(*this->begin(), *other.begin());
			two_product(result[0], result[1]);
			return result;
		} else if constexpr(N > 1 && M > 1) {
			auto static constexpr N1 = N / 2, N2 = N - N1, M1 = M / 2, M2 = M - M1;
			auto a1 = Exact<N1>();
			auto a2 = Exact<N2>();
			auto b1 = Exact<M1>();
			auto b2 = Exact<M2>();
			this->partition(a1, a2);
			other.partition(b1, b2);
			return (a1 * b1 + a1 * b2) + (a2 * b1 + a2 * b2);
		}
	}
};

#endif
