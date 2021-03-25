#ifndef ENDIAN_HPP
#define ENDIAN_HPP

enum endian_type {
	big_endian     = 0x00000001,
	little_endian  = 0x01000000,
	pdp_endian     = 0x00010000,
	unknown_endian = 0xffffffff,
};

#if defined(__BIG_ENDIAN__)
	enum { byte_order = big_endian };
#elif defined(__LITTLE_ENDIAN__)
	enum { byte_order = little_endian };
#else
	#ifdef BSD
		#include <sys/endian.h>
	#else
		#include <endian.h>
	#endif
	#if __BYTE_ORDER == __BIG_ENDIAN
		enum { byte_order = big_endian };
	#elif __BYTE_ORDER == __LITTLE_ENDIAN
		enum { byte_order = little_endian };
	#else
		#error unsupported endianness
	#endif
#endif

struct Endian {
	static bool constexpr big    = byte_order == big_endian;
	static bool constexpr little = byte_order == little_endian;
};

#endif
