#ifndef EXACT_HPP
#define EXACT_HPP

#include "ieee754.hpp"
#include <cstddef>
#include <array>

template <std::size_t N = 1>
class Exact : std::array<double, N> {
	using Array = std::array<double, N>;

	template <std::size_t M>
	friend class Exact;

	auto split(double &l, double &h) const {
		static constexpr auto bits = IEEE754::bits();
		static constexpr auto s = 1ul + (1ul << (bits + 1u) / 2u);
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
		const auto hv = x - l;
		const auto lv = x - hv;
		const auto hr = h - hv;
		const auto lr = l - lv;
		l = lr + hr, h = x;
	};

	Exact() = default;
	Exact(double l, double h) : Array{{l, h}} { }

public:
	Exact(double d) : Array{{d}} { }

	auto operator>(const int &zero) const { return this->back() > zero; }
	auto operator<(const int &zero) const { return this->back() < zero; }
	auto operator>=(const int &zero) const { return this->back() >= zero; }
	auto operator<=(const int &zero) const { return this->back() <= zero; }
	auto operator!=(const int &zero) const { return this->back() != zero; }

	template <std::size_t M>
	auto operator+(const Exact<M> &other) const {
		auto result = Exact<M+N>();
		auto h = result.begin();
		for (const auto &e: *this)
			*h++ = e;
		for (const auto &f: other)
			*h++ = f;
		for (std::size_t m = 0; m < M; ++m)
			for (std::size_t n = 0; n < N; ++n)
				two_sum(result[m+n], result[m+N]);
		return result;
	}

	template <std::size_t M>
	auto operator*(const Exact<M> &other) const {
		if constexpr (N == 1 && M == 1) {
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
