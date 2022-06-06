#include <stdio.h>
#include <string>
#include <fstream>
#include "TokenParser.h"
#include "CPU.h"

using namespace std;

enum class NumberType { None, Binary, Hexadecimal, Decimal };
enum class Command { None, Origin, Export };

int main(int argc, char** argv)
{
	TokenParser tokenParser = TokenParser();

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
			printf("WARNING! : Expected input file! Using test.asm as default\n");
			filename = (char*)"..\\Homebrew_Assembler\\default.asm";
		}
		else
		{
			char* filename = argv[1];
		}

		// Print a welcome message
		printf("\n\nWelcome to the Homebrew CPU Assembler - v1.0!\n\n");

		// Print to screen the name of the file we are parsin
		printf("Parsing file: %s\n", filename);

		// Used to keep track of line that is currently being parsed
		int linenum = 0;
		string line;

		// Need to keep track of address for writing to ROM
		int address = 0;

		// Open the file for input
		ifstream inFile(filename);
		if (inFile.is_open())
		{
			// Pull in each line individually...will repeat until EOF
			while (getline(inFile, line))
			{
				//-----------------------------------------------------------------------------------------------------
				// *** PARSING PASS 0: Convert input file into tokens delimited by space, tab, or commas ***
				//-----------------------------------------------------------------------------------------------------

				// Token parse object needs to be reset for every line!
				tokenParser.Reset();

				// Convert the line that was pulled in into an array of characters
				const char* c_line = line.c_str();

				// Echo current line to screen
				printf("\n %d: \"%s\"\n", linenum, c_line);

				// Parse initial tokens
				tokenParser.Parse(c_line, " ,\t");

				int nTokens = tokenParser.NumTokens();
				LineType lineType = tokenParser.GetLineType();
				vector<char*> tokens = tokenParser.GetTokens();

				printf("   ->*** %s ***\n", tokenParser.GetLineTypeString());

				Command currCommand = Command::None;
				for (int i = 0; i < nTokens; i++)
				{
					printf("   -> %s\n", tokens[i]);

					if (!strcmp(tokens[i], ORIGIN))
					{
						currCommand = Command::Origin;
						printf("      -- origin directive\n");
					}
					else if (!strcmp(tokens[i], EXPORT))
					{
						currCommand = Command::Export;
						printf("      -- export directive\n");
					}

					if (currCommand == Command::Origin && strcmp(tokens[i], ORIGIN))
					{
						int base = -1;
						char* arg = tokens[i];

						switch (tokens[i][0])
						{
						case BIN_CHAR[0]:
							base = 2;
							arg = strtok(arg, BIN_CHAR);
							break;

						case HEX_CHAR[0]:
							base = 16;
							arg = strtok(arg, HEX_CHAR);
							break;

						default:
							base = 10;
							break;
						}

						if (base != -1)
						{
							address = stoi(arg, nullptr, base);
							printf("      -- Address = %x\n", address);

							currCommand = Command::None;
						}
						else
						{
							printf("ERROR occurred in parsing argument for ORIGIN directive\n");
						}
					}
				}

				// Increase line number
				linenum++;
			}

			printf("\n\nDONE!\n");

			// Once we reach EOF, close the file
			inFile.close();
		}
		else
		{
			printf("Unable to open file!\n");
		}
	}

	return 0;
}