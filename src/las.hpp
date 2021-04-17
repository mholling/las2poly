////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LAS_HPP
#define LAS_HPP

#include "endian.hpp"
#include "point.hpp"
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <string>

class LAS {
	std::istream &input;
	std::size_t position;
	double x_scale, y_scale, z_scale;
	double x_offset, y_offset, z_offset;
	std::uint8_t point_data_record_format;

	void read_values() { }

	template <typename Arg, typename ...Args>
	void read_values(Arg &arg, Args &...args) {
		input.read(reinterpret_cast<char *>(&arg), sizeof(arg));
		position += sizeof(arg);
		if constexpr (Endian::big && sizeof(arg) > 1)
			std::reverse(reinterpret_cast<char *>(&arg), reinterpret_cast<char *>(&arg) + sizeof(arg));
		read_values(args...);
	}

	template <typename ...Args>
	void read_ahead(std::size_t offset, Args &...args) {
		input.ignore(offset);
		position += offset;
		read_values(args...);
	}

public:
	std::size_t size;

	LAS(std::istream &input) : input(input), position(4) {
		std::uint8_t version_major, version_minor;
		std::uint32_t offset_to_point_data;
		std::uint32_t legacy_number_of_point_records;
		std::uint64_t number_of_point_records;

		read_ahead(20, version_major, version_minor);
		read_ahead(70, offset_to_point_data);
		read_ahead(4, point_data_record_format);
		read_ahead(2, legacy_number_of_point_records);
		read_ahead(20, x_scale, y_scale, z_scale, x_offset, y_offset, z_offset);

		if (point_data_record_format > 127)
			throw std::runtime_error("LAZ format not supported");
		if (point_data_record_format > 10)
			throw std::runtime_error("unsupported LAS point data record format");
		if (version_major != 1)
			throw std::runtime_error("unsupported LAS version " + std::to_string(version_major) + "." + std::to_string(version_minor));

		if (version_minor < 4)
			size = legacy_number_of_point_records;
		else {
			read_ahead(68, number_of_point_records);
			size = number_of_point_records;
		}

		read_ahead(offset_to_point_data - position);
	}

	auto read() {
		double x, y, z;
		unsigned char classification;
		bool key_point, withheld, overlap;
		char buffer[67];

		switch (point_data_record_format) {
		case 0:  input.read(buffer, 20); break;
		case 1:  input.read(buffer, 28); break;
		case 2:  input.read(buffer, 26); break;
		case 3:  input.read(buffer, 34); break;
		case 4:  input.read(buffer, 57); break;
		case 5:  input.read(buffer, 63); break;
		case 6:  input.read(buffer, 30); break;
		case 7:  input.read(buffer, 36); break;
		case 8:  input.read(buffer, 38); break;
		case 9:  input.read(buffer, 59); break;
		case 10: input.read(buffer, 67); break;
		}

		if constexpr (Endian::big) {
			std::reverse(buffer,     buffer + 4);
			std::reverse(buffer + 4, buffer + 8);
			std::reverse(buffer + 8, buffer + 12);
		}
		x = x_offset + x_scale * *reinterpret_cast<std::int32_t *>(buffer);
		y = y_offset + y_scale * *reinterpret_cast<std::int32_t *>(buffer + 4);
		z = z_offset + z_scale * *reinterpret_cast<std::int32_t *>(buffer + 8);

		switch (point_data_record_format) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b01000000;
			withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b10000000;
			classification = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00011111;
			overlap        = 12 == classification;
			break;
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00000010;
			withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00000100;
			overlap        = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00001000;
			classification = *reinterpret_cast<std::uint8_t *>(buffer + 17);
		}

		return Point(x, y, z, classification, key_point, withheld, overlap);
	}
};

#endif
