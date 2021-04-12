#ifndef EXACT_HPP
#define EXACT_HPP

#include "ieee754.hpp"
#include <cstddef>
#include <array>
#include <compare>

// partial implementation of:
//     Shewchuk, J. 'Adaptive Precision Floating-Point
//     Arithmetic and Fast Robust Geometric Predicates'

template <std::size_t N = 1>
class Exact : std::array<double, N> {
	using Array = std::array<double, N>;

	template <std::size_t M>
	friend class Exact;

	auto split(double &l, double &h) const {
		constexpr auto s = 1ul + (1ul << (IEEE754::bits() + 1u) / 2u);
		const auto &a = this->back();
		const auto c = s * a;
		const auto aa = c - a;
		h = c - aa, l = a - h;
	};

	template <std::size_t M>
	auto partition(Exact<M> &e1, Exact<N-M> &e2) const {
		auto here = this->begin();
		for (auto &e: e1)
			e = *here++;
		for (auto &e: e2)
			e = *here++;
	}

	static void two_sum(double &l, double &h) {
		const auto x = l + h;
		const auto lv = x - h;
		const auto hv = x - lv;
		const auto lr = l - lv;
		const auto hr = h - hv;
		l = lr + hr, h = x;
	};

	static void two_diff(double &l, double &h) {
		const auto x = l - h;
		const auto hv = l - x;
		const auto lv = x + hv;
		const auto hr = h - hv;
		const auto lr = l - lv;
		l = lr - hr, h = x;
	};

	static void fast_two_sum(double &l, double &h) {
		const auto x = l + h;
		const auto lv = x - h;
		const auto lr = l - lv;
		l = lr, h = x;
	}

	Exact() = default;
	Exact(double l, double h) : Array{{l, h}} { }

public:
	Exact(double d) : Array{{d}} { }

	friend auto operator<=>(const Exact &e1, const Exact &e2) {
		return (e1 - e2).back() <=> 0;
	}

	friend auto operator>(const Exact &e, const int &zero) {
		return e.back() > zero;
	}

	template <std::size_t M>
	auto operator+(const Exact<M> &other) const {
		auto result = Exact<M+N>();
		if constexpr (N == 2 && M == 2) { // EXPANSION-SUM
			auto h = result.begin();
			for (const auto &e: *this)
				*h++ = e;
			for (const auto &f: other)
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
	auto operator-(const Exact<M> &other) const { // EXPANSION-DIFF
		auto result = Exact<M+N>();
		auto h = result.begin();
		for (const auto &e: *this)
			*h++ = e;
		for (const auto &f: other)
			*h++ = f;
		for (std::size_t m = 0; m < M; ++m) {
			two_diff(result[m], result[m+N]);
			for (std::size_t n = 1; n < N; ++n)
				two_sum(result[m+n], result[m+N]);
		}
		return result;
	}

	template <std::size_t M>
	auto operator*(const Exact<M> &other) const {
		if constexpr (N == 1 && M == 1) { // TWO-PRODUCT
			double al, ah, bl, bh;
			this->split(al, ah);
			other.split(bl, bh);
			const auto h = this->back() * other.back();
			const auto err1 = h - (ah * bh);
			const auto err2 = err1 - (al * bh);
			const auto err3 = err2 - (ah * bl);
			const auto l = (al * bl) - err3;
			return Exact<M+N>(l, h);
		} else if constexpr(N > 1 && M > 1) {
			constexpr auto N1 = N / 2, N2 = N - N1, M1 = M / 2, M2 = M - M1;
			Exact<N1> a1;
			Exact<N2> a2;
			Exact<M1> b1;
			Exact<M2> b2;
			this->partition(a1, a2);
			other.partition(b1, b2);
			return (a1 * b1 + a1 * b2) + (a2 * b1 + a2 * b2);
		}
	}
};

#endif
