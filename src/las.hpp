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
#include <stdexcept>
#include <functional>
#include <limits>
#include <optional>
#include <deque>
#include <variant>
#include <algorithm>

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

class LAS {
	std::istream &input;
	std::size_t position;
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
	std::uint32_t chunk_size;

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
		if (absolute_position > position)
			read_ahead(absolute_position - position);
		else if (absolute_position < position)
			input.seekg(position = absolute_position);
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

	struct LASPointReader {
		LAS &las;

		LASPointReader(LAS &las) : las(las) { }

		void operator()(char *buffer) {
			las.input.read(buffer, las.point_data_record_length);
			las.position += las.point_data_record_length;
		}
	};

	template <typename Decompressor, int record_length>
	struct LAZPointReader {
		using Callback = std::function<void(unsigned char *, std::size_t)>;
		auto static constexpr chunk_size_variable = std::numeric_limits<uint32_t>::max();

		LAS &las;
		Callback callback;
		std::size_t extra_bytes;
		std::optional<Decompressor> decompressor;
		std::deque<uint64_t> chunk_points;
		std::deque<uint64_t> chunk_lengths;
		std::deque<uint64_t> chunk_offsets;

		LAZPointReader(LAS &las) :
			las(las),
			callback([&](unsigned char *buffer, std::size_t length) {
				this->las.input.read(reinterpret_cast<char *>(buffer), length);
				this->las.position += length;
			}),
			extra_bytes(las.point_data_record_length - record_length)
		{
			std::int64_t chunk_table_offset;
			std::uint32_t version;
			std::uint32_t chunk_count;

			las.read_values(chunk_table_offset);
			if (chunk_table_offset < 0)
				throw std::runtime_error("invalid LAZ file");
			las.read_to(chunk_table_offset);
			las.read_values(version, chunk_count);

			if (version != 0)
				throw std::runtime_error("invalid LAZ file");
			if (las.chunk_size == 0 && las.size > 0)
				throw std::runtime_error("invalid LAZ file");
			if (chunk_count == 0 && las.size > 0)
				throw std::runtime_error("invalid LAZ file");

			auto cstream = lazperf::InCbStream(callback);
			auto decoder = lazperf::decoders::arithmetic(cstream);
			auto decompressor = lazperf::decompressors::integer(32, 2);

			decoder.readInitBytes();
			decompressor.init();

			chunk_points.push_back(0);
			chunk_lengths.push_back(0);
			chunk_offsets.push_back(sizeof(std::int64_t) + las.offset_to_point_data);

			auto remaining = las.size;
			for ( ; chunk_count > 0; --chunk_count) {
				if (las.chunk_size == chunk_size_variable)
					chunk_points.push_back(decompressor.decompress(decoder, chunk_points.back(), 0));
				else if (remaining < las.chunk_size)
					chunk_points.push_back(remaining);
				else
					chunk_points.push_back(las.chunk_size);

				if (chunk_points.back() > remaining)
					throw std::runtime_error("invalid LAZ file");
				else
					remaining -= chunk_points.back();

				chunk_lengths.push_back(decompressor.decompress(decoder, chunk_lengths.back(), 1));
				chunk_offsets.push_back(chunk_offsets.back() + chunk_lengths.back());
			}

			if (remaining > 0)
				throw std::runtime_error("invalid LAZ file");
		}

		void operator()(char *buffer) {
			if (chunk_points.front() == 0) {
				decompressor.emplace(callback, extra_bytes);
				las.read_to(chunk_offsets.front());
				chunk_offsets.pop_front();
				chunk_points.pop_front();
			}
			decompressor->decompress(buffer);
			--chunk_points.front();
		}
	};

	using LAZPointReader0 = LAZPointReader<lazperf::point_decompressor_0, 20>;
	using LAZPointReader1 = LAZPointReader<lazperf::point_decompressor_1, 28>;
	using LAZPointReader2 = LAZPointReader<lazperf::point_decompressor_2, 26>;
	using LAZPointReader3 = LAZPointReader<lazperf::point_decompressor_3, 34>;
	using LAZPointReader6 = LAZPointReader<lazperf::point_decompressor_6, 30>;
	using LAZPointReader7 = LAZPointReader<lazperf::point_decompressor_7, 36>;
	using LAZPointReader8 = LAZPointReader<lazperf::point_decompressor_8, 38>;

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
		position(4),
		chunk_size(0),
		point_reader(LASPointReader(*this))
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

		std::array<std::uint16_t, 11> static constexpr minimum_record_length = {{20,28,26,34,57,63,30,36,38,59,67}};
		if (point_data_record_length < minimum_record_length[point_data_record_format])
			throw std::runtime_error("invalid LAS file");
		else
			buffer_string.resize(point_data_record_length);

		if (version_minor < 4)
			size = legacy_number_of_point_records;
		else {
			read_ahead(56, start_of_extended_variable_length_records, number_of_extended_variable_length_records, number_of_point_records);
			size = number_of_point_records;
		}

		read_to(header_size);
		read_vlrs<std::uint16_t>(number_of_variable_length_records);

		if (version_minor == 4) {
			read_to(start_of_extended_variable_length_records);
			read_vlrs<std::uint64_t>(number_of_extended_variable_length_records);
		}

		read_to(offset_to_point_data);

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
		double x, y, z;
		unsigned char classification;
		bool key_point, withheld, overlap;

		auto const buffer = buffer_string.data();
		read_point_record(buffer);

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
			key_point      = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00000010;
			withheld       = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00000100;
			overlap        = *reinterpret_cast<std::uint8_t *>(buffer + 15) & 0b00001000;
			classification = *reinterpret_cast<std::uint8_t *>(buffer + 16);
		}
		return Point(x, y, z, classification, key_point, withheld, overlap);
	}
};

#endif
