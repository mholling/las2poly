#ifndef ENDIAN_HPP
#define ENDIAN_HPP

enum endian_type {
	big_endian     = 0x00000001,
	little_endian  = 0x01000000,
	pdp_endian     = 0x00010000,
	unknown_endian = 0xffffffff,
};

#if defined(__BIG_ENDIAN__)
	constexpr auto byte_order = big_endian;
#elif defined(__LITTLE_ENDIAN__)
	constexpr auto byte_order = little_endian;
#else
	#ifdef BSD
		#include <sys/endian>
	#else
		#include <endian>
	#endif
	#if __BYTE_ORDER == __BIG_ENDIAN
		constexpr auto byte_order = big_endian;
	#elif __BYTE_ORDER == __LITTLE_ENDIAN
		constexpr auto byte_order = little_endian;
	#else
		#error unsupported endianness
	#endif
#endif

namespace Endian {
	static constexpr bool big    = byte_order == big_endian;
	static constexpr bool little = byte_order == little_endian;
};

#endif
