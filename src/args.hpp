#ifndef ARGS_HPP
#define ARGS_HPP

#include <stdexcept>
#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <ios>
#include <iostream>

class Args {
	struct InvalidArgument : std::runtime_error {
		InvalidArgument(const std::string message, std::string arg) : runtime_error(message + " " + arg) { }
	};

	struct Option {
		std::string letter;
		std::string name;
		std::string format;
		std::string description;
		std::function<void(std::string)> callback;

		auto operator==(const std::string &arg) const {
			return arg == letter || arg == name;
		}
	};

	struct Position {
		std::string format;
		std::string description;
		std::function<void(std::string)> callback;
	};

	std::string command, banner;
	std::vector<std::string> args;
	std::vector<Option> options;
	std::vector<Position> positions;

	auto help() const {
		std::stringstream help;
		help << command << " - " << banner << std::endl;
		help << "  usage: " << command;
		if (!options.empty())
			help << " [options]";
		for (const auto &position: positions)
			help << " " << position.format;
		help << std::endl << "  options:" << std::endl;
		std::size_t letter_width = 0, name_width = 0, format_width = 0;
		for (const auto &option: options) {
			letter_width = std::max(letter_width, option.letter.length());
			name_width = std::max(name_width, option.name.length());
			format_width = std::max(format_width, option.format.length());
		}
		for (const auto &option: options)
			help << "    " << std::left
				<< std::setw(letter_width) << option.letter << ", "
				<< std::setw(name_width + 1) << option.name
				<< std::setw(format_width + 2) << option.format
				<< option.description << std::endl;
		return help.str();
	}

public:
	Args(int argc, char *argv[], std::string banner) : banner(banner) {
		command = std::filesystem::path(*argv++).filename();
		for (--argc; argc > 0; --argc)
			args.push_back(*argv++);
		if (args.empty())
			args.push_back("--help");
	}

	template <typename T>
	void option(std::string letter, std::string name, std::string format, std::string description, T &value) {
		std::stringstream description_with_default;
		description_with_default << description << " (default: " << value << ")";
		options.push_back(Option({letter, name, format, description_with_default.str(), [&](auto arg) {
			std::stringstream(arg) >> value;
		}}));
	}

	template <typename T>
	void option(std::string letter, std::string name, std::string description, T &value) {
		options.push_back(Option({letter, name, "", description, [&](auto arg) {
			std::stringstream(arg) >> value;
		}}));
	}

	template <typename T>
	void position(std::string format, std::string description, T &value) {
		positions.push_back(Position({format, description, [&](auto arg) {
			std::stringstream(arg) >> value;
		}}));
	}

	bool parse() {
		options.push_back(Option({"-h", "--help", "", "show this help summary", [&](auto) {
			std::cout << help();
		}}));

		auto position = positions.begin();
		for (auto arg = args.begin(); arg != args.end(); ++arg) {
			const auto option = std::find(options.begin(), options.end(), *arg);
			if (option == options.end()) {
				if (arg->rfind("-", 0) == 0)
					throw InvalidArgument("invalid option:", *arg);
				if (position == positions.end())
					throw InvalidArgument("invalid argument:", *arg);
				(position++)->callback(*arg);
			} else if (option->format.empty()) {
				option->callback("1");
				if (option->name == "--help")
					return false;
			} else if (arg + 1 == args.end()) {
				throw InvalidArgument("missing argument for option:", *arg);
			} else {
				option->callback(*++arg);
			}
		}

		if (position != positions.end())
			throw InvalidArgument("missing argument:", position->description);
		return true;
	}
};

#endif
