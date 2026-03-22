#include "test-CommandLine.h"
#include <cstdint>
#include <iostream>
#include <limits>

int main(int argc, char* argv[])
{
	// These variables can be set via the command line.
	std::string oString = "Default Value";
	int32_t oInteger = -1;
	uint32_t oUnsigned = 0;
	double oDouble = 0.0;
	float oFloat = 0.f;
	bool oBool = false;
	bool oPrintHelp = false;
	// First, configure all possible command line options.
	CommandLine args("A demonstration of the simple command line parser.");
	args.addArgument({"-s", "--string"}, &oString, "A string value");
	args.addArgument({"-i", "--integer"}, &oInteger, "A integer value");
	args.addArgument({"-u", "--unsigned"}, &oUnsigned, "A unsigned value");
	args.addArgument({"-d", "--double"}, &oDouble, "A float value");
	args.addArgument({"-f", "--float"}, &oFloat, "A double value");
	args.addArgument({"-b", "--bool"}, &oBool, "A bool value");
	args.addArgument(
		{"-h", "--help"}, &oPrintHelp,
		"Print this help. This help message is actually so long "
		"that it requires a line break!"
	);

	// Then do the actual parsing.
	try
	{
		args.parse(argc, argv);
	}
	catch (std::runtime_error const& e)
	{
		std::cout << e.what() << std::endl;
		args.printHelp();
		return 1;
	}
	// When oPrintHelp was set to true, we print a help message and exit.
	if (oPrintHelp)
	{
		args.printHelp();
		return 0;
	}
	// Print the resulting values.
	std::cout << "mString: " << oString << std::endl;
	std::cout << "mInteger: " << oInteger << std::endl;
	std::cout << "mUnsigned: " << oUnsigned << std::endl;
	std::cout.precision(std::numeric_limits<double>::max_digits10);
	std::cout << "mDouble: " << oDouble << std::endl;
	std::cout.precision(std::numeric_limits<float>::max_digits10);
	std::cout << "mFloat: " << oFloat << std::endl;
	std::cout << "mBool: " << oBool << std::endl;
	//
	return 0;
}