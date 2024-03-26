////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef LAS_HPP
#define LAS_HPP

#include "lazperf/lazperf.hpp"
#include "lazperf/streams.hpp"
#include "lazperf/decoder.hpp"
#include "lazperf/decompressor.hpp"

#include "srs.hpp"
#include "point.hpp"
#include <bit>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <deque>
#include <limits>
#include <stdexcept>
#include <optional>
#include <variant>
#include <algorithm>

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

class LAS {
	std::istream &input;
	std::string buffer_string;

	std::uint8_t version_major, version_minor;
	std::uint16_t header_size;
	std::uint32_t offset_to_point_data;
	std::uint32_t number_of_variable_length_records;
	std::uint8_t point_data_record_format;
	std::uint16_t point_data_record_length;
	std::uint32_t legacy_number_of_point_records;
	double x_scale, y_scale, z_scale;
	double x_offset, y_offset, z_offset;
	std::uint64_t start_of_extended_variable_length_records;
	std::uint32_t number_of_extended_variable_length_records;
	std::uint64_t number_of_point_records;

	std::size_t extra_bytes;
	std::uint32_t chunk_size;

	void read_values() { }

	template <int N, typename ...Args>
	void read_values(std::array<char, N> &arg, Args &...args) {
		input.read(arg.data(), N);
		read_values(args...);
	}

	template <typename ...Args>
	void read_values(std::string &arg, Args &...args) {
		input.read(arg.data(), arg.size());
		read_values(args...);
	}

	template <typename Arg, typename ...Args>
	void read_values(Arg &arg, Args &...args) {
		input.read(reinterpret_cast<char *>(&arg), sizeof(arg));
		if constexpr (std::endian::native == std::endian::big && sizeof(arg) > 1)
			std::reverse(reinterpret_cast<char *>(&arg), reinterpret_cast<char *>(&arg) + sizeof(arg));
		read_values(args...);
	}

	template <typename ...Args>
	void read_ahead(std::size_t offset, Args &...args) {
		input.seekg(offset, std::ios_base::cur);
		read_values(args...);
	}

	void seek_to(std::size_t position) {
		input.seekg(position);
	}

	template <typename LengthType>
	void read_vlrs(std::uint32_t number_of_variable_length_records) {
		for (auto record = 0ul; record < number_of_variable_length_records; ++record) {
			std::array<char, 16> static constexpr  laszip_encoded_user_id = {"laszip encoded"};
			std::array<char, 16> static constexpr lasf_projection_user_id = {"LASF_Projection"};
			std::array<char, 16> user_id;
			std::uint16_t record_id;
			LengthType record_length_after_header;
			read_ahead(2, user_id, record_id, record_length_after_header);
			read_ahead(32);

			if (22204 == record_id && laszip_encoded_user_id == user_id) { // LASzip metadata
				std::uint16_t compressor;
				read_values(compressor);
				if (compressor != (point_data_record_format < 6 ? 2 : 3))
					throw std::runtime_error("invalid LAZ file");
				read_ahead(10, chunk_size);
				read_ahead(record_length_after_header - 16);
			} else if (2112 == record_id && lasf_projection_user_id == user_id) { // OGC coordinate system WKT
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
			} else if (34735 == record_id && lasf_projection_user_id == user_id) { // GeoKeyDirectoryTag
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

	using Callback = std::function<void(unsigned char *, std::size_t)>;
	Callback laz_callback;

	std::deque<uint64_t> chunk_points;
	std::deque<uint64_t> chunk_lengths;
	std::deque<uint64_t> chunk_offsets;

	void read_chunk_table() {
		auto constexpr max_chunk_size = std::numeric_limits<uint32_t>::max();
		auto const variable_chunk_size = max_chunk_size == chunk_size;

		std::int64_t chunk_table_offset;
		std::uint32_t version;
		std::uint32_t chunk_count;

		read_values(chunk_table_offset);
		if (chunk_table_offset < 0)
			throw std::runtime_error("invalid LAZ file");
		seek_to(chunk_table_offset);
		read_values(version, chunk_count);

		if (version != 0)
			throw std::runtime_error("invalid LAZ file");
		if (chunk_size == 0 && size > 0)
			throw std::runtime_error("invalid LAZ file");
		if (chunk_count == 0 && size > 0)
			throw std::runtime_error("invalid LAZ file");

		auto cstream = lazperf::InCbStream(laz_callback);
		auto decoder = lazperf::decoders::arithmetic(cstream);
		auto decompressor = lazperf::decompressors::integer(32, 2);

		decoder.readInitBytes();
		decompressor.init();

		chunk_points.push_back(0);
		chunk_lengths.push_back(0);
		chunk_offsets.push_back(sizeof(std::int64_t) + offset_to_point_data);

		for (auto remaining = size; chunk_count > 0; --chunk_count) {
			if (variable_chunk_size)
				chunk_points.push_back(decompressor.decompress(decoder, chunk_points.back(), 0));
			else if (remaining < chunk_size)
				chunk_points.push_back(remaining);
			else
				chunk_points.push_back(chunk_size);

			if (chunk_points.back() > remaining)
				throw std::runtime_error("invalid LAZ file");
			else
				remaining -= chunk_points.back();

			if (chunk_count == 1 && remaining > 0)
				throw std::runtime_error("invalid LAZ file");

			chunk_lengths.push_back(decompressor.decompress(decoder, chunk_lengths.back(), 1));
			chunk_offsets.push_back(chunk_offsets.back() + chunk_lengths.back());
		}
	}

	template <typename Decompressor>
	void read_buffer(char *buffer, Decompressor &decompressor) {
		while (chunk_points.front() == 0) {
			decompressor.emplace(laz_callback, extra_bytes);
			seek_to(chunk_offsets.front());
			chunk_offsets.pop_front();
			chunk_points.pop_front();
		}
		decompressor->decompress(buffer);
		--chunk_points.front();
	}

	void read_buffer(char *buffer) {
		input.read(buffer, point_data_record_length);
	}

	template <typename Decompressor>
	struct LAZPointReader {
		LAS &las;
		std::optional<Decompressor> decompressor;

		LAZPointReader(LAS &las) : las(las) {
			las.read_chunk_table();
		}

		void operator()(char *buffer) {
			las.read_buffer(buffer, decompressor);
		}
	};

	struct LASPointReader {
		LAS &las;

		LASPointReader(LAS &las) : las(las) { }

		void operator()(char *buffer) {
			las.read_buffer(buffer);
		}
	};

	using LAZPointReader0 = LAZPointReader<lazperf::point_decompressor_0>;
	using LAZPointReader1 = LAZPointReader<lazperf::point_decompressor_1>;
	using LAZPointReader2 = LAZPointReader<lazperf::point_decompressor_2>;
	using LAZPointReader3 = LAZPointReader<lazperf::point_decompressor_3>;
	using LAZPointReader6 = LAZPointReader<lazperf::point_decompressor_6>;
	using LAZPointReader7 = LAZPointReader<lazperf::point_decompressor_7>;
	using LAZPointReader8 = LAZPointReader<lazperf::point_decompressor_8>;

	std::variant<
		LASPointReader,
		LAZPointReader0,
		LAZPointReader1,
		LAZPointReader2,
		LAZPointReader3,
		LAZPointReader6,
		LAZPointReader7,
		LAZPointReader8
	> point_reader;

	void read_point_record(char *buffer) {
		auto const read = [&](auto &point_reader) { point_reader(buffer); };
		std::visit(read, point_reader);
	}

public:
	std::size_t size;
	OptionalSRS srs;

	LAS(std::istream &input) :
		input(input),
		chunk_size(0),
		laz_callback([&](unsigned char *buffer, std::size_t length) {
			input.read(reinterpret_cast<char *>(buffer), length);
		}),
		point_reader(std::in_place_type<LASPointReader>, *this)
	{
		read_ahead(20, version_major, version_minor);
		read_ahead(68, header_size, offset_to_point_data, number_of_variable_length_records, point_data_record_format, point_data_record_length, legacy_number_of_point_records);
		read_ahead(20, x_scale, y_scale, z_scale, x_offset, y_offset, z_offset);

		bool const compressed = point_data_record_format & 0b10000000;
		point_data_record_format &= 0b01111111;

		if (point_data_record_format > 10)
			throw std::runtime_error("unsupported LAS point data record format");
		if (version_major != 1)
			throw std::runtime_error("unsupported LAS version " + std::to_string(version_major) + "." + std::to_string(version_minor));

		std::array<std::uint16_t, 11> static constexpr minimum_record_lengths = {{20,28,26,34,57,63,30,36,38,59,67}};
		if (auto const minimum_length = minimum_record_lengths[point_data_record_format]; point_data_record_length < minimum_length)
			throw std::runtime_error("invalid LAS file");
		else
			extra_bytes = point_data_record_length - minimum_length;
		buffer_string.resize(point_data_record_length);

		if (version_minor < 4)
			size = legacy_number_of_point_records;
		else {
			read_ahead(56, start_of_extended_variable_length_records, number_of_extended_variable_length_records, number_of_point_records);
			size = number_of_point_records;
		}

		seek_to(header_size);
		read_vlrs<std::uint16_t>(number_of_variable_length_records);

		if (version_minor == 4) {
			seek_to(start_of_extended_variable_length_records);
			read_vlrs<std::uint64_t>(number_of_extended_variable_length_records);
		}

		seek_to(offset_to_point_data);

		if (compressed)
			switch (point_data_record_format) {
			case 4:
			case 5:
			case 9:
			case 10:
				throw std::runtime_error("unsupported LAS point data record format");
			case 0: point_reader.emplace<LAZPointReader0>(*this); break;
			case 1: point_reader.emplace<LAZPointReader1>(*this); break;
			case 2: point_reader.emplace<LAZPointReader2>(*this); break;
			case 3: point_reader.emplace<LAZPointReader3>(*this); break;
			case 6: point_reader.emplace<LAZPointReader6>(*this); break;
			case 7: point_reader.emplace<LAZPointReader7>(*this); break;
			case 8: point_reader.emplace<LAZPointReader8>(*this); break;
			}
	}

	auto read() {
		auto const buffer = buffer_string.data();
		read_point_record(buffer);

		if constexpr (std::endian::native == std::endian::big) {
			std::reverse(buffer,     buffer + 4);
			std::reverse(buffer + 4, buffer + 8);
			std::reverse(buffer + 8, buffer + 12);
		}
		double x = x_offset + x_scale * *reinterpret_cast<std::int32_t *>(buffer);
		double y = y_offset + y_scale * *reinterpret_cast<std::int32_t *>(buffer + 4);
		double z = z_offset + z_scale * *reinterpret_cast<std::int32_t *>(buffer + 8);

		unsigned char classification;
		bool key_point, withheld, overlap;

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
			key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00000010;
			withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00000100;
			overlap        = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00001000;
			classification = *reinterpret_cast<std::uint8_t *>(buffer + 16);
		}
		return Point(x, y, z, classification, key_point, withheld, overlap);
	}
};

#endif
