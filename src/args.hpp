////////////////////////////////////////////////////////////////////////////////
// Copyright 2021 Matthew Hollingworth.
// Distributed under GNU General Public License version 3.
// See LICENSE file for full license information.
////////////////////////////////////////////////////////////////////////////////

#ifndef ARGS_HPP
#define ARGS_HPP

#include <stdexcept>
#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include <iostream>
#include <codecvt>
#include <cstddef>
#include <algorithm>
#include <iomanip>
#include <optional>
#include <filesystem>
#include <type_traits>
#include <utility>

class Args {
	struct InvalidArgument : std::runtime_error {
		InvalidArgument(std::string const message, std::string const arg) : runtime_error(message + " " + arg) { }
	};

	using Callback = std::function<void(std::string)>;

	struct Option {
		std::string letter;
		std::string name;
		std::string format;
		std::string description;
		unsigned format_wsize;
		unsigned format_delta;
		Callback callback;

		Option(std::string letter, std::string name, std::string format, std::string description, Callback callback) : letter(letter), name(name), format(format), description(description), callback(callback) {
			auto convert = std::wstring_convert<std::codecvt_utf8<wchar_t>>();
			format_wsize = convert.from_bytes(format).size();
			format_delta = format.size() - format_wsize;
		}

		auto operator==(std::string const &arg) const {
			return arg == letter || arg == name;
		}
	};

	struct Position {
		bool variadic;
		std::string format;
		std::string description;
		Callback callback;

		Position(bool variadic, std::string format, std::string description, Callback callback) :
			variadic(variadic),
			format(format),
			description(description),
			callback(callback)
		{ }
	};

	std::string command, banner;
	std::vector<std::string> args;
	std::vector<Option> options;
	std::vector<Position> positions;

	auto help() const {
		auto help = std::stringstream();
		help << command << " - " << banner << std::endl;
		help << "  usage: " << command;
		if (!options.empty())
			help << " [options]";
		for (auto const &position: positions)
			if (position.variadic)
				help << " " << position.format << " [" << position.format << " ...]";
			else
				help << " " << position.format;
		help << std::endl << "  options:" << std::endl;
		auto letter_width = 0u, name_width = 0u, format_width = 0u;
		for (auto const &option: options) {
			letter_width = std::max<unsigned>(letter_width, option.letter.size());
			name_width   = std::max<unsigned>(name_width,   option.name.size());
			format_width = std::max<unsigned>(format_width, option.format_wsize);
		}
		for (auto const &option: options)
			help << "    " << std::left
				<< std::setw(letter_width) << option.letter << (option.letter.empty() ? "  " : ", ")
				<< std::setw(name_width + 1) << option.name
				<< std::setw(option.format_delta + format_width + 1) << option.format
				<< option.description << std::endl;
		return help.str();
	}

public:
	Args(int argc, char *argv[], std::string banner) : banner(banner) {
		command = std::filesystem::path(*argv++).filename();
		for (--argc; argc > 0; --argc)
			args.push_back(*argv++);
	}

	template <typename Value>
	void option(std::string letter, std::string name, std::string format, std::string description, std::optional<Value> &optional) {
		auto description_with_default = std::stringstream();
		if (optional)
			description_with_default << description << " (default: " << *optional << ")";
		else
			description_with_default << description;
		options.emplace_back(letter, name, format, description_with_default.str(), [&](auto arg) {
			if constexpr (std::is_same_v<Value, std::filesystem::path>)
				optional.emplace(arg);
			else if (auto parser = std::stringstream(arg); !(parser >> optional.emplace()) || !parser.eof())
				throw std::runtime_error("invalid argument: " + arg);
		});
	}

	template <typename Value>
	void option(std::string letter, std::string name, std::string format, std::string description, std::optional<std::vector<Value>> &optional) {
		auto description_with_default = std::stringstream();
		description_with_default << description;
		if (optional) {
			auto before = " (default: ";
			for (auto const &value: *optional)
				description_with_default << std::exchange(before, ",") << value;
			description_with_default << ")";
		}
		options.emplace_back(letter, name, format, description_with_default.str(), [&](auto arg) {
			optional.emplace();
			auto list = std::stringstream(arg);
			for (std::string arg; std::getline(list, arg, ','); )
				if (auto parser = std::stringstream(arg); !(parser >> optional->emplace_back()) || !parser.eof())
					throw std::runtime_error("invalid argument: " + arg);
		});
	}

	void option(std::string letter, std::string name, std::string description, std::optional<bool> &optional) {
		option(letter, name, "", description, optional);
	}

	void position(std::string format, std::string description, std::filesystem::path &path) {
		positions.emplace_back(false, format, description, [&](auto arg) {
			path.assign(arg);
		});
	}

	void position(std::string format, std::string description, std::vector<std::filesystem::path> &paths) {
		for (auto const &position: positions)
			if (position.variadic)
				throw std::runtime_error(format + ": only one variadic positional argument allowed");
		positions.emplace_back(true, format, description, [&](auto arg) {
			paths.emplace_back(arg);
		});
	}

	void version(std::string version_string) {
		options.emplace_back("-v", "--version", "", "show program version", [&](auto) {
			std::cout << version_string << std::endl;
		});
	}

	template <typename Validate>
	bool parse(Validate const &validate) {
		options.emplace_back("-h", "--help", "", "show this help summary", [&](auto) {
			std::cout << help();
		});

		auto position_args = std::vector<std::string>();
		auto option_args = std::vector<std::string>();

		try {
			for (auto arg = args.begin(); arg != args.end(); ) {
				auto const option = std::find(options.begin(), options.end(), *arg);
				if (option == options.end())
					if (arg->rfind("-", 0) == 0 && arg->size() > 1)
						throw InvalidArgument("invalid option:", *arg);
					else
						position_args.push_back(*arg++);
				else if (option->format.empty() || arg + 1 == args.end())
					option_args.push_back(*arg++);
				else {
					option_args.push_back(*arg++);
					option_args.push_back(*arg++);
				}
			}

			for (auto arg = option_args.begin(); arg != option_args.end(); ++arg) {
				auto const option = std::find(options.begin(), options.end(), *arg);
				if (option->format.empty())
					option->callback("1");
				else if (arg + 1 == option_args.end())
					throw InvalidArgument("missing argument for option:", *arg);
				else
					option->callback(*++arg);
				if (option->name == "--help" || option->name == "--version")
					return false;
			}

			auto position = positions.begin();
			for (auto arg = position_args.begin(); arg != position_args.end(); )
				if (position == positions.end())
					throw InvalidArgument("invalid argument:", *arg);
				else if (!position->variadic)
					(position++)->callback(*arg++);
				else if (position_args.end() - arg > positions.end() - position)
					position->callback(*arg++);
				else if (position_args.end() - arg == positions.end() - position)
					(position++)->callback(*arg++);
				else
					++position;

			if (position != positions.end() && position->variadic)
				++position;
			if (position != positions.end())
				throw InvalidArgument("missing argument:", position->description);
			validate();
		} catch (std::runtime_error &error) {
			std::cerr << help();
			throw error;
		}

		return true;
	}
};

#endif
