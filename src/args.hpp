#ifndef ARGS_HPP
#define ARGS_HPP

#include <stdexcept>
#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstddef>
#include <algorithm>
#include <iomanip>
#include <optional>
#include <filesystem>
#include <utility>

class Args {
	struct InvalidArgument : std::runtime_error {
		InvalidArgument(const std::string message, std::string arg) : runtime_error(message + " " + arg) { }
	};

	using Callback = std::function<void(std::string)>;

	struct Option {
		std::string letter;
		std::string name;
		std::string format;
		std::string description;
		Callback callback;

		Option(std::string letter, std::string name, std::string format, std::string description, Callback callback) : letter(letter), name(name), format(format), description(description), callback(callback) { }

		auto operator==(const std::string &arg) const {
			return arg == letter || arg == name;
		}
	};

	struct Position {
		bool variadic;
		std::string format;
		std::string description;
		Callback callback;

		Position(bool variadic, std::string format, std::string description, Callback callback) : variadic(variadic), format(format), description(description), callback(callback) { }
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
		for (const auto &position: positions)
			if (position.variadic)
				help << " " << position.format << " [" << position.format << " ...]";
			else
				help << " " << position.format;
		help << std::endl << "  options:" << std::endl;
		auto letter_width = 0u, name_width = 0u, format_width = 0u;
		for (const auto &option: options) {
			letter_width = std::max<unsigned>(letter_width, option.letter.length());
			name_width = std::max<unsigned>(name_width, option.name.length());
			format_width = std::max<unsigned>(format_width, option.format.length());
		}
		for (const auto &option: options)
			help << "    " << std::left
				<< std::setw(letter_width) << option.letter << (option.letter.empty() ? "  " : ", ")
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
	}

	template <typename Value>
	void option(std::string letter, std::string name, std::string format, std::string description, std::optional<Value> &optional) {
		auto description_with_default = std::stringstream();
		if (optional)
			description_with_default << description << " (default: " << optional.value() << ")";
		else
			description_with_default << description;
		options.emplace_back(letter, name, format, description_with_default.str(), [&](auto arg) {
			auto parser = std::stringstream(arg);
			if (!(parser >> optional.emplace()) || !parser.eof())
				throw std::runtime_error("invalid argument: " + arg);
		});
	}

	template <typename Value>
	void option(std::string letter, std::string name, std::string format, std::string description, std::optional<std::vector<Value>> &optional) {
		auto description_with_default = std::stringstream();
		description_with_default << description;
		if (optional) {
			auto before = " (default: ";
			for (const auto &value: optional.value())
				description_with_default << std::exchange(before, ",") << value;
			description_with_default << ")";
		}
		options.emplace_back(letter, name, format, description_with_default.str(), [&](auto arg) {
			optional.emplace();
			auto list = std::stringstream(arg);
			for (std::string arg; std::getline(list, arg, ','); ) {
				auto parser = std::stringstream(arg);
				if (!(parser >> optional.value().emplace_back()) || !parser.eof())
					throw std::runtime_error("invalid argument: " + arg);
			}
		});
	}

	void option(std::string letter, std::string name, std::string description, std::optional<bool> &optional) {
		option(letter, name, "", description, optional);
	}

	template <typename Value>
	void position(std::string format, std::string description, Value &value) {
		positions.emplace_back(false, format, description, [&](auto arg) {
			std::stringstream(arg) >> value;
		});
	}

	template <typename Value>
	void position(std::string format, std::string description, std::vector<Value> &values) {
		for (const auto &position: positions)
			if (position.variadic)
				throw std::runtime_error(format + ": only one variadic positional argument allowed");
		positions.emplace_back(true, format, description, [&](auto arg) {
			std::stringstream(arg) >> values.emplace_back();
		});
	}

	void version(std::string version_string) {
		options.emplace_back("-v", "--version", "", "show program version", [&](auto) {
			std::cout << version_string << std::endl;
		});
	}

	template <typename Validate>
	bool parse(Validate validate) {
		options.emplace_back("-h", "--help", "", "show this help summary", [&](auto) {
			std::cout << help();
		});

		auto position_args = std::vector<std::string>();
		auto option_args = std::vector<std::string>();

		try {
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
