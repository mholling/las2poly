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
#include <optional>
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
		bool variadic;
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
			if (position.variadic)
				help << " " << position.format << " [" << position.format << " ...]";
			else
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
				<< std::setw(format_width + 1) << option.format
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
	void option(std::string letter, std::string name, std::string format, std::string description, std::optional<T> &optional) {
		std::stringstream description_with_default;
		if (optional)
			description_with_default << description << " (default: " << optional.value() << ")";
		else
			description_with_default << description;
		options.push_back(Option({letter, name, format, description_with_default.str(), [&](auto arg) {
			T value;
			std::stringstream(arg) >> value;
			optional = value;
		}}));
	}

	template <typename T>
	void option(std::string letter, std::string name, std::string format, std::string description, std::optional<std::vector<T>> &optional) {
		options.push_back(Option({letter, name, format, description, [&](auto arg) {
			std::vector<T> values;
			auto list = std::stringstream(arg);
			for (std::string arg; std::getline(list, arg, ','); )
				std::stringstream(arg) >> values.emplace_back();
			optional = values;
		}}));
	}

	void option(std::string letter, std::string name, std::string description, std::optional<bool> &optional) {
		option(letter, name, "", description, optional);
	}

	template <typename T>
	void position(std::string format, std::string description, T &value) {
		positions.push_back(Position({false, format, description, [&](auto arg) {
			std::stringstream(arg) >> value;
		}}));
	}

	template <typename T>
	void position(std::string format, std::string description, std::vector<T> &values) {
		for (const auto &position: positions)
			if (position.variadic)
				throw std::runtime_error(format + ": only one variadic positional argument allowed");
		positions.push_back(Position({true, format, description, [&](auto arg) {
			std::stringstream(arg) >> values.emplace_back();
		}}));
	}

	void version(std::string version_string) {
		options.push_back(Option({"-v", "--version", "", "show program version", [&](auto) {
			std::cout << version_string << std::endl;
		}}));
	}

	bool parse() {
		options.push_back(Option({"-h", "--help", "", "show this help summary", [&](auto) {
			std::cout << help();
		}}));

		std::vector<std::string> position_args, option_args;

		for (auto arg = args.begin(); arg != args.end(); ) {
			const auto option = std::find(options.begin(), options.end(), *arg);
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
			const auto option = std::find(options.begin(), options.end(), *arg);
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
		for (auto arg = position_args.begin(); arg != position_args.end(); ++arg)
			if (position == positions.end())
				throw InvalidArgument("invalid argument:", *arg);
			else if (!position->variadic)
				(position++)->callback(*arg);
			else if (position_args.end() - arg > positions.end() - position)
				position->callback(*arg);
			else
				(position++)->callback(*arg);

		if (position != positions.end())
			throw InvalidArgument("missing argument:", position->description);

		return true;
	}
};

#endif
