#include "Parser.h"

int main(int argc, char** argv)
{
	// Create the parser object
	Parser parser = Parser();

	// Print a welcome message
	printf("\n\nWelcome to the Homebrew CPU Assembler - v1.0!\n\n");

	// Currently limited to two input arguments, though this will probably change soon
	if (argc > 2)
	{
		printf("ERROR! : Too many arguments provided!\n");
		return 0;
	}
	else
	{
		// The file that will be parsed
		char* filename = NULL;

		// If no input file is provided in the command line, just parse demo.asm by default
		if (argc < 2)
		{
			printf("WARNING! : Expected input file! Using demo.asm as default\n");
			filename = (char*)"..\\Homebrew_Assembler\\demo.asm";
		}
		else
		{
			// Otherwise, use the file provided by the user
			char* filename = argv[1];
		}

		// While in development, I'll keep console printing to verbose so that we can keep an eye
		// on the inner-workings of the assembler
		parser.SetMode(Mode::Verbose);

		// Initiate parsing of file
		parser.Parse(filename);
	}

	return 0;
}