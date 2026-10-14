#include "test-CommandLine.h"
#include <algorithm>
#include <variant>
#include <iomanip>
#include <sstream>
#include <stdexcept>

CommandLine::CommandLine(std::string description)
	: _description(std::move(description))
{}

void CommandLine::addArgument(std::vector<std::string> const& flags, Value const& value, std::string const& help)
{
	_arguments.emplace_back(Argument{flags, value, help});
}

void CommandLine::printHelp(std::ostream& os) const
{

	// Print the general description.
	os << _description << std::endl;

	// Find the argument with the longest combined flag length (to align the help messages).
	uint32_t maxFlagLength = 0;

	for (auto const& argument: _arguments)
	{
		uint32_t flagLength = 0;
		for (auto const& flag: argument._flags)
		{
			// Plus comma and space.
			flagLength += static_cast<uint32_t>(flag.size()) + 2;
		}

		maxFlagLength = std::max(maxFlagLength, flagLength);
	}
	// Now print each argument.
	for (auto const& argument: _arguments)
	{
		std::string flags;
		for (auto const& flag: argument._flags)
		{
			flags += flag + ", ";
		}
		// Remove last comma and space and add padding according to the longest flags in order to align the help messages.
		std::stringstream sstr;
		sstr << std::left << std::setw(maxFlagLength) << flags.substr(0, flags.size() - 2);
		// Print the help for each argument. This is a bit more involved
		// since we do line wrapping for long descriptions.
		size_t spacePos = 0;
		size_t lineWidth = 0;
		while (spacePos != std::string::npos)
		{
			size_t nextspacePos = argument._help.find_first_of(' ', spacePos + 1);
			sstr << argument._help.substr(spacePos, nextspacePos - spacePos);
			lineWidth += nextspacePos - spacePos;
			spacePos = nextspacePos;

			if (lineWidth > 60)
			{
				os << sstr.str() << std::endl;
				sstr = std::stringstream();
				sstr << std::left << std::setw(maxFlagLength - 1) << " ";
				lineWidth = 0;
			}
		}
	}
}

void CommandLine::parse(int argc, char* argv[]) const
{
	// Skip the first argument (name of the program).
	int i = 1;
	while (i < argc)
	{
		// First, identify whether the value is separated by a space or a '='.
		std::string flag(argv[i]);
		std::string value;
		bool valueIsSeparate = false;

		// If there is an equals sign ('=') in the flag, the part after the '=' is actually
		// the value.
		size_t equalPos = flag.find('=');
		if (equalPos != std::string::npos)
		{
			value = flag.substr(equalPos + 1);
			flag = flag.substr(0, equalPos);
		}
		// Else the following argument is the value.
		else if (i + 1 < argc)
		{
			value = argv[i + 1];
			valueIsSeparate = true;
		}
		// Search for an argument with the provided flag.
		bool foundArgument = false;
		//
		for (auto const& argument: _arguments)
		{
			if (std::find(argument._flags.begin(), argument._flags.end(), flag) != std::end(argument._flags))
			{
				foundArgument = true;
				// In the case of booleans, there must not be a value present.
				// So if the value is neither 'true' nor 'false' it is considered to be the next argument.
				if (std::holds_alternative<bool*>(argument._value))
				{
					if (!value.empty() && value != "true" && value != "false")
					{
						valueIsSeparate = false;
					}
					*std::get<bool*>(argument._value) = (value != "false");
				}
				// In all other cases there must be a value.
				else if (value.empty())
				{
					throw std::runtime_error(
						"Failed to parse command line arguments: "
						"Missing value for argument \"" +
						flag + "\"!"
					);
				}
				// For a std::string, we take the entire value.
				else if (std::holds_alternative<std::string*>(argument._value))
				{
					*std::get<std::string*>(argument._value) = value;
				}
				// In all other cases we use a 'std::stringstream' to convert the value.
				else
				{
					std::visit(
						[&value](auto&& arg) {
							std::stringstream sstr(value);
							sstr >> *arg;
						},
						argument._value
					);
				}

				break;
			}
		}
		// Print a warning if there was an unknown argument.
		if (!foundArgument)
		{
			std::cerr << "Ignoring unknown command line argument \"" << flag << "\"." << std::endl;
		}
		// Advance to the next flag.
		++i;
		// If the value was separated, we have to advance our index once more.
		if (foundArgument && valueIsSeparate)
		{
			++i;
		}
	}
}
