////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LAS_HPP
#define LAS_HPP

#include "srs.hpp"
#include "point.hpp"
#include <bit>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <string>

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

class LAS {
	std::istream &input;
	std::size_t position;

	std::uint8_t version_major, version_minor;
	std::uint16_t header_size;
	std::uint32_t offset_to_point_data;
	std::uint32_t number_of_variable_length_records;
	std::uint8_t point_data_record_format;
	std::uint32_t legacy_number_of_point_records;
	double x_scale, y_scale, z_scale;
	double x_offset, y_offset, z_offset;
	std::uint64_t start_of_extended_variable_length_records;
	std::uint32_t number_of_extended_variable_length_records;
	std::uint64_t number_of_point_records;

	void read_values() { }

	template <int N, typename ...Args>
	void read_values(std::array<char, N> &arg, Args &...args) {
		input.read(arg.data(), N);
		position += N;
		read_values(args...);
	}

	template <typename ...Args>
	void read_values(std::string &arg, Args &...args) {
		input.read(arg.data(), arg.size());
		position += arg.size();
		read_values(args...);
	}

	template <typename Arg, typename ...Args>
	void read_values(Arg &arg, Args &...args) {
		input.read(reinterpret_cast<char *>(&arg), sizeof(arg));
		position += sizeof(arg);
		if constexpr (std::endian::native == std::endian::big && sizeof(arg) > 1)
			std::reverse(reinterpret_cast<char *>(&arg), reinterpret_cast<char *>(&arg) + sizeof(arg));
		read_values(args...);
	}

	template <typename ...Args>
	void read_ahead(std::size_t offset, Args &...args) {
		input.ignore(offset);
		position += offset;
		read_values(args...);
	}

	void read_to(std::size_t absolute_position) {
		read_ahead(absolute_position - position);
	}

	template <typename LengthType>
	void read_vlrs(std::uint32_t number_of_variable_length_records) {
		for (auto record = 0ul; !srs && record < number_of_variable_length_records; ++record) {
			std::array<char, 16> static constexpr lasf_projection_user_id = {"LASF_Projection"};
			std::array<char, 16> user_id;
			std::uint16_t record_id;
			LengthType record_length_after_header;
			read_ahead(2, user_id, record_id, record_length_after_header);
			read_ahead(32);

			if (lasf_projection_user_id != user_id)
				read_ahead(record_length_after_header);
			else if (2112 == record_id) { // OGC coordinate system WKT
				auto compound_wkt = std::string(record_length_after_header, '\0');
				read_values(compound_wkt);
				if (auto const projcs = compound_wkt.find("PROJCS["); projcs != std::string::npos) {
					auto begin = compound_wkt.begin() + projcs, end = begin + 7;
					for (auto count = 1; ; '[' == *end ? ++count : ']' == *end ? --count : 0, ++end) {
						if (0 == count)
							srs.emplace(begin, end);
						else if (compound_wkt.end() != end)
							continue;
						break;
					}
				}
			} else if (34735 == record_id) { // GeoKeyDirectoryTag
				std::uint16_t key_directory_version, key_revision, minor_revision, number_of_keys;
				read_values(key_directory_version, key_revision, minor_revision, number_of_keys);
				for (auto key = 0u; key < number_of_keys; ++key) {
					std::uint16_t key_id, tiff_tag_location, count, value_offset;
					read_values(key_id, tiff_tag_location, count, value_offset);
					if (3072 == key_id) // ProjectedCSTypeGeoKey
						try { srs.emplace(value_offset); }
						catch (SRS::InvalidEPSG &) { }
				}
				read_ahead(record_length_after_header - 8 * (number_of_keys + 1));
			} else
				read_ahead(record_length_after_header);
		}
	}

public:
	std::size_t size;
	OptionalSRS srs;

	LAS(std::istream &input) : input(input), position(4) {
		read_ahead(20, version_major, version_minor);
		read_ahead(68, header_size, offset_to_point_data, number_of_variable_length_records, point_data_record_format);
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
			read_ahead(56, start_of_extended_variable_length_records, number_of_extended_variable_length_records, number_of_point_records);
			size = number_of_point_records;
		}

		read_to(header_size);
		read_vlrs<std::uint16_t>(number_of_variable_length_records);
		read_to(offset_to_point_data);
	}

	auto read() {
		double x, y, z;
		unsigned char classification;
		bool key_point, withheld, overlap;
		char buffer[67];

		switch (point_data_record_format) {
		case 0:  input.read(buffer, 20); position += 20; break;
		case 1:  input.read(buffer, 28); position += 28; break;
		case 2:  input.read(buffer, 26); position += 26; break;
		case 3:  input.read(buffer, 34); position += 34; break;
		case 4:  input.read(buffer, 57); position += 57; break;
		case 5:  input.read(buffer, 63); position += 63; break;
		case 6:  input.read(buffer, 30); position += 30; break;
		case 7:  input.read(buffer, 36); position += 36; break;
		case 8:  input.read(buffer, 38); position += 38; break;
		case 9:  input.read(buffer, 59); position += 59; break;
		case 10: input.read(buffer, 67); position += 67; break;
		}

		if constexpr (std::endian::native == std::endian::big) {
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
		default:
			key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00000010;
			withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00000100;
			overlap        = *reinterpret_cast<std::uint8_t *>(buffer + 16) & 0b00001000;
			classification = *reinterpret_cast<std::uint8_t *>(buffer + 17);
		}

		return Point(x, y, z, classification, key_point, withheld, overlap);
	}

	void finalise() {
		if (version_minor < 4)
			return;
		read_to(start_of_extended_variable_length_records);
		read_vlrs<std::uint64_t>(number_of_extended_variable_length_records);
	}
};

#endif
