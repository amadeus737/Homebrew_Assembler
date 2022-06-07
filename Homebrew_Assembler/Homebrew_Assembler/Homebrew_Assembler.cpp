#include <stdio.h>
#include <string>
#include <fstream>
#include "Parser.h"
#include "Config.h"

using namespace std;

int main(int argc, char** argv)
{
	Parser parser = Parser();

	if (argc > 2)
	{
		printf("ERROR! : Too many arguments provided!\n");
		return 0;
	}
	else
	{
		// The file that will be parsed
		char* filename = NULL;

		if (argc < 2)
		{
			printf("WARNING! : Expected input file! Using demo.asm as default\n");
			filename = (char*)"..\\Homebrew_Assembler\\demo.asm";
		}
		else
		{
			char* filename = argv[1];
		}

		// Print a welcome message
		printf("\n\nWelcome to the Homebrew CPU Assembler - v1.0!\n\n");

		parser.SetMode(Mode::Verbose);
		parser.Parse(filename);
	}

	return 0;
}
