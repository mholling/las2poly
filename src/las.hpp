#ifndef LAS_HPP
#define LAS_HPP

#include "record.hpp"
#include <iostream>
#include <cstddef>
#include <stdexcept>
#include <string>

class LAS {
	std::istream &input;
	std::size_t position;
	double x_scale, y_scale, z_scale;
	double x_offset, y_offset, z_offset;
	std::uint8_t point_data_record_format;

	void read() { }

	template <typename Arg, typename ...Args>
	void read(Arg &arg, Args &...args) {
		input.read(reinterpret_cast<char *>(&arg), sizeof(arg));
		position += sizeof(arg);
		read(args...);
	}

	template <typename ...Args>
	void read_ahead(std::size_t offset, Args &...args) {
		input.ignore(offset);
		position += offset;
		read(args...);
	}

public:
	std::size_t count;

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
			count = legacy_number_of_point_records;
		else {
			read_ahead(68, number_of_point_records);
			count = number_of_point_records;
		}

		read_ahead(offset_to_point_data - position);
	}

	auto record() {
		Record record;
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

		record.x = x_offset + x_scale * *reinterpret_cast<std::int32_t *>(buffer);
		record.y = y_offset + y_scale * *reinterpret_cast<std::int32_t *>(buffer + 4);
		record.z = z_offset + z_scale * *reinterpret_cast<std::int32_t *>(buffer + 8);
		switch (point_data_record_format) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			record.key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b01000000;
			record.withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b10000000;
			record.classification = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00011111;
			record.overlap        = 12 == record.classification;
			break;
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			record.key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00000010;
			record.withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00000100;
			record.overlap        = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00001000;
			record.classification = *reinterpret_cast<std::uint8_t *>(buffer + 17);
		}

		return record;
	}
};

#endif
