////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef SHAPEFILE_HPP
#define SHAPEFILE_HPP

#include "polygons.hpp"
#include <limits>
#include <bit>
#include <optional>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>

static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

class Shapefile {
	using EPSG = std::optional<int>;

	template <typename Value, unsigned offset>
	void static little(char *data, Value value) {
		*reinterpret_cast<Value *>(data + offset) = value;
		if constexpr (std::endian::native == std::endian::big)
			std::reverse(data + offset, data + offset + sizeof(Value));
	}

	template <typename Value, unsigned offset>
	void static big(char *data, Value value) {
		*reinterpret_cast<Value *>(data + offset) = value;
		if constexpr (std::endian::native == std::endian::little)
			std::reverse(data + offset, data + offset + sizeof(Value));
	}

	class SHPX {
		using Integer = std::int32_t;
		using Double = double;

		std::filesystem::path shp_path;
		std::filesystem::path shx_path;

	public:
		SHPX(std::filesystem::path const &shp_path) : shp_path(shp_path), shx_path(shp_path) {
			shx_path.replace_extension(".shx");
		}

		void operator()(Polygons const &polygons) {
			auto static constexpr file_header_size = 100ull;
			auto static constexpr record_header_size = 8ull;
			auto static constexpr content_prefix_size = 44ull;
			auto static constexpr file_magic = 9994;
			auto static constexpr file_version = 1000;
			auto static constexpr shape_type = 5;

			auto const bounds = Bounds(polygons);
			auto const shx_file_length = file_header_size + polygons.size() * record_header_size;
			auto const shp_file_length = std::accumulate(polygons.begin(), polygons.end(), file_header_size, [](auto const &sum, auto const &polygon) {
				return std::accumulate(polygon.begin(), polygon.end(), sum + record_header_size + content_prefix_size, [](auto const &sum, auto const &ring) {
					return sum + sizeof(Integer) + (ring.size() + 1ull) * sizeof(Double) * 2ull;
				});
			});

			if (shp_file_length > sizeof(std::uint16_t) * INT32_MAX)
				throw std::runtime_error("too many points for shapefile format");

			char file_header[file_header_size] = {0};
			   big<Integer,  0>(file_header, file_magic);
			little<Integer, 28>(file_header, file_version);
			little<Integer, 32>(file_header, shape_type);
			little<Double,  36>(file_header, bounds.xmin);
			little<Double,  44>(file_header, bounds.ymin);
			little<Double,  52>(file_header, bounds.xmax);
			little<Double,  60>(file_header, bounds.ymax);
			little<Double,  68>(file_header, 0.0); // zmin
			little<Double,  76>(file_header, 0.0); // zmax
			little<Double,  84>(file_header, 0.0); // mmin
			little<Double,  92>(file_header, 0.0); // mmax

			auto shp_file = std::ofstream(shp_path, std::ios_base::binary);
			auto shx_file = std::ofstream(shx_path, std::ios_base::binary);
			shp_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			shx_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

			big<Integer, 24>(file_header, shx_file_length / sizeof(std::uint16_t));
			shx_file.write(file_header, file_header_size);

			big<Integer, 24>(file_header, shp_file_length / sizeof(std::uint16_t));
			shp_file.write(file_header, file_header_size);

			for (auto id = 0ull, record_offset = file_header_size; auto const &polygon: polygons) {
				auto const num_parts = polygon.size();
				auto const num_points = std::accumulate(polygon.begin(), polygon.end(), 0ull, [](auto const &sum, auto const &ring) {
					return sum + ring.size() + 1ull;
				});
				auto const content_length = content_prefix_size + num_parts * sizeof(Integer) + num_points * sizeof(Double) * 2;
				auto const bounds = Bounds(polygon);

				char record_header[record_header_size];
				big<Integer, 0>(record_header, record_offset / sizeof(std::uint16_t));
				big<Integer, 4>(record_header, content_length / sizeof(std::uint16_t));

				shx_file.write(record_header, record_header_size);
				record_offset += record_header_size + content_length;

				big<Integer, 0>(record_header, ++id);
				shp_file.write(record_header, record_header_size);

				char content_prefix[content_prefix_size];
				little<Integer,  0>(content_prefix, shape_type);
				little< Double,  4>(content_prefix, bounds.xmin);
				little< Double, 12>(content_prefix, bounds.ymin);
				little< Double, 20>(content_prefix, bounds.xmax);
				little< Double, 28>(content_prefix, bounds.ymax);
				little<Integer, 36>(content_prefix, num_parts);
				little<Integer, 40>(content_prefix, num_points);

				auto indices = std::vector<char>(num_parts * sizeof(Integer));
				auto coords = std::vector<char>(num_points * sizeof(Double) * 2);
				auto index = reinterpret_cast<Integer *>(indices.data());
				auto coord = reinterpret_cast<Double *>(coords.data());

				for (auto count = 0ul; auto const &ring: polygon) {
					*index++ = count;
					count += ring.size() + 1ul;
					for (auto const &[x, y]: ring)
						*coord++ = x, *coord++ = y;
					auto const &[x, y] = ring.front();
					*coord++ = x, *coord++ = y;
				}

				if constexpr (std::endian::native == std::endian::big) {
					for (auto pos = indices.begin(); pos != indices.end(); pos += sizeof(Integer))
						std::reverse(pos, pos + sizeof(Integer));
					for (auto pos = coords.begin(); pos != coords.end(); pos += sizeof(Double))
						std::reverse(pos, pos + sizeof(Double));
				}

				shp_file.write(content_prefix, content_prefix_size);
				shp_file.write(indices.data(), indices.size());
				shp_file.write(coords.data(), coords.size());
			}
		}

		operator bool() const {
			return std::filesystem::exists(shp_path) || std::filesystem::exists(shx_path);
		}
	};

	class DBF {
		std::filesystem::path dbf_path;

	public:
		DBF(std::filesystem::path const &shp_path) : dbf_path(shp_path) {
			dbf_path.replace_extension(".dbf");
		}

		void operator()(Polygons const &polygons) {
			auto static constexpr header_size = 65;
			auto static constexpr field_width = 1 + std::numeric_limits<std::uint32_t>::digits10;

			auto const now = std::chrono::system_clock::now();
			auto const time = std::chrono::system_clock::to_time_t(now);
			auto const &local = *std::localtime(&time);

			char header[header_size] = {0};
			header[0] = 0x03;                // version number
			header[1] = local.tm_year;       // year
			header[2] = local.tm_mon + 1;    // month
			header[3] = local.tm_mday;       // day
			header[8] = header_size;         // length of header
			header[10] = field_width + 1;    // length of record
			header[29] = 0x57;               // ANSI language driver
			header[32] = 'F';                //
			header[33] = 'I';                // field name
			header[34] = 'D';                //
			header[43] = 'N';                // field type
			header[48] = field_width;        // field width
			header[64] = 0x0d;               // terminator

			little<std::uint32_t, 4>(header, polygons.size());

			auto file = std::ofstream(dbf_path, std::ios_base::binary);
			file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			file.write(header, header_size);

			for (auto fid = 0ul; fid <= polygons.size(); ++fid)
				file << '\x20' << std::setfill(' ') << std::setw(field_width) << fid;
			file << '\x1a';
		}

		operator bool() const {
			return std::filesystem::exists(dbf_path);
		}
	};

	SHPX shpx;
	DBF dbf;

public:
	Shapefile(std::filesystem::path const &shp_path, EPSG const &epsg) : shpx(shp_path), dbf(shp_path) {
		if (epsg)
			throw std::runtime_error("can't store EPSG for shapefile format");
	}

	void operator()(Polygons const &polygons) {
		if (polygons.size() >= INT32_MAX)
			throw std::runtime_error("too many polygons for shapefile format");
		shpx(polygons);
		dbf(polygons);
	}

	operator bool() const {
		return shpx || dbf;
	}
};

#endif
