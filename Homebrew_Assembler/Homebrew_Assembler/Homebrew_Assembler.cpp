#include <stdio.h>
#include <string>
#include <fstream>
#include "FileParser.h"
#include "Config.h"

using namespace std;

enum class NumberType { None, Binary, Hexadecimal, Decimal };
enum class Command { None, Origin, Export, Symbol, Label, OpCode };

// Forward declare functions
void CalculateBase(char* tokens, int* base, char** arg);
bool GetLabel(char *c, string &label, int &replacementValue);
bool IsNumeric(char* c);
string labels[10];
int replacementValues[10];
int main(int argc, char** argv)
{
	FileParser fileParser = FileParser();

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
		int startAddress = 0;
		int endAddress = 0;				
		char* oc_mnemonic = NULL;
		int labelCount= -1;
		Command lastCommand = Command::None;

		// Open the file for input
		ifstream inFile(filename);
		if (inFile.is_open())
		{
			// Pull in each line individually...will repeat until EOF
			while (getline(inFile, line))
			{
				//--------------------------------------------------------------------------------------
				// *** PARSING PASS 0: Parse lines into tokens using space, tab, or comma delimiters ***
				//--------------------------------------------------------------------------------------

				// Line parse object needs to be reset for every line!
				fileParser.Reset();

				// Convert the line that was pulled in into an array of characters
				const char* c_line = line.c_str();

				// Echo current line to screen
				printf("\n %d: \"%s\"\n", linenum, c_line);

				// Parse initial tokens
				fileParser.Parse(c_line, " ,\t");

				int nTokens = fileParser.NumTokens();
				LineType lineType = fileParser.GetLineType();
				vector<char*> tokens = fileParser.GetTokens();

				//printf("   ->*** %s ***\n", fileParser.GetLineTypeString());

				char* label = NULL;
				Command currCommand = Command::None;
				for (int i = 0; i < nTokens; i++)
				{
					//printf("   -> %s\n", tokens[i]);

					char* directive_parse = strtok(tokens[i], DIRECTIVE_KEYS);
					char* symbol_parse = strtok(tokens[i], SYMBOL_KEYS);
					char* label_parse = strtok(tokens[i], LABEL_KEYS);

					//printf(" ....dir parse : %s\n", directive_parse);
					//printf(" ....sym parse : %s\n", symbol_parse);
					//printf(" ....lbl parse: %s\n", label_parse);

					if (!strcmp(directive_parse, ORIGIN_STR))
					{
						if (lastCommand == Command::OpCode)
						{
							printf("      -- Still need to process last no-arg opcode!\n");
							printf("      -- Address = %x\n", address);
						}

						currCommand = Command::Origin;
						//printf("      -- origin directive\n");
					}
					else if (!strcmp(directive_parse, EXPORT_STR))
					{
						if (lastCommand == Command::OpCode)
						{
							printf("      -- Still need to process last no-arg opcode!\n");
							printf("      -- Address = %x\n", address);
						}

						currCommand = Command::Export;
						//printf("      -- export directive\n");
					}
					else if (strcmp(symbol_parse, tokens[i]))
					{
						if (lastCommand == Command::OpCode)
						{
							printf("      -- Still need to process last no-arg opcode!\n");
							printf("      -- Address = %x\n", address);
						}

						currCommand = Command::Symbol;
						//printf("      -- symbol define\n");
					
						labelCount++;
						label = symbol_parse;
						labels[labelCount] = label;
						//printf("label is %s\n", labels[labelCount].c_str());
						//printf("      --> label saved as %s\n", newEntry.label);
					}
					else if (strcmp(label_parse, tokens[i]))
					{
						if (lastCommand == Command::OpCode)
						{
							printf("      -- Still need to process last no-arg opcode!\n");
							printf("      -- Address = %x\n", address);
						}

						currCommand = Command::Label;
						//printf("      -- label define\n");
						
						labelCount++;
						label = label_parse;
						labels[labelCount] = label;
						//printf("label is %s\n", labels[labelCount].c_str());
						//printf("      --> label saved as %s\n", newEntry.label);
					}

					// Only other command to process is an OpCode!
					if (lineType == LineType::OpCode && currCommand != Command::OpCode)
					{
						// Need to find a way to handle previous no-arg opcodes here too!

						currCommand = Command::OpCode;
						//lastCommand = Command::OpCode;
						//printf("      -- opcode define\n");

						// Later, replace with actual size of opcode
						address += 8;
						
						oc_mnemonic = tokens[i];
					}
				
					int base;
					char* arg = tokens[i];

					CalculateBase(tokens[i], &base, &arg);

					if (currCommand == Command::Origin && strcmp(directive_parse, ORIGIN_STR))
					{
						if (base != -1)
						{
							address = stoi(arg, nullptr, base);
							printf("      -- Address = %x\n", address);

							lastCommand = currCommand;
							currCommand = Command::None;
						}
						else
						{
							printf("ERROR occurred in parsing argument for ORIGIN directive\n");
						}
					}

					if (currCommand == Command::Export && strcmp(directive_parse, EXPORT_STR))
					{
						if (base != -1)
						{
							startAddress = stoi(arg, nullptr, base);
							printf("      -- Start Address = %x\n", startAddress);
						}
						else
						{
							printf("ERROR occurred in parsing argument for EXPORT directive\n");
						}

						CalculateBase(tokens[i+1], &base, &arg);
						if (base != -1)
						{
							endAddress = stoi(arg, nullptr, base);
							printf("      -- End Address = %x\n", endAddress);

							lastCommand = currCommand;
							currCommand = Command::None;
						}
						else
						{
							printf("ERROR occurred in parsing argument for EXPORT directive\n");
						}
					}

					if (currCommand == Command::Symbol && !strcmp(symbol_parse, tokens[i]))
					{
						if (base != -1)
						{
							int replacementValue = stoi(arg, nullptr, base);
							printf("      -- Symbol: %s = %x\n", label, replacementValue);
							replacementValues[labelCount] = replacementValue;

							lastCommand = currCommand;
							currCommand = Command::None;
						}
					}

					if (currCommand == Command::Label)
					{
						if (base != -1)
						{
							int replacementValue = address;
							printf("      -- Label: %s = %x\n", label, replacementValue);

							replacementValues[labelCount] = replacementValue;

							lastCommand = currCommand;
							currCommand = Command::None;
						}
					}

					if (currCommand == Command::OpCode)
					{
						lastCommand = Command::OpCode;

						if (strcmp(arg, oc_mnemonic))
						{
							lastCommand = Command::None;
							//	printf("A %s\n", arg);
							//	printf("o %s\n", oc_mnemonic);

							if (IsNumeric(arg))
							{
								if (base != -1)
								{
									int arg0 = stoi(arg, nullptr, base);
									printf("      -- arg0 = %x\n", arg0);

									address += 8;
								}
							}
							else
							{
								string label = "";
								int replacementValue = 0;
								if (!GetLabel(arg, label, replacementValue))
									printf("      -- arg0 is a REGISTER %s\n", arg);
								else
								{ 
									printf("      -- arg0 is a LABEL %x\n", replacementValue);
									address += 8;
								}
							}
							
							CalculateBase(tokens[i + 1], &base, &arg);
							if (IsNumeric(arg))
							{
								if (base != -1)
								{
									int arg1 = stoi(arg, nullptr, base);
									printf("      -- arg1 = %x\n", arg1);

									address += 8;
								}
							}
							else
							{
								string label = "";
								int replacementValue = 0;
								if (!GetLabel(arg, label, replacementValue))
									printf("      -- arg1 is a REGISTER %s\n", arg);
								else
								{ 
									address += 8;
									printf("      -- arg1 is a LABEL %x\n", replacementValue);
								}
							}
							
							printf("      -- Address = %x\n", address);

							lastCommand = currCommand;
							currCommand = Command::None;
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

bool IsNumeric(char* c)
{	
	bool number = false;

	// Test if equal to 0-9
	for (int i = 0; i < 10; i++)
	{
		number = number || !strcmp(c, to_string(i).c_str());
	}

	// Also need to test for number type prefixes
	number = number || !strcmp(c, BIN_KEY);
	number = number || !strcmp(c, HEX_KEY);
	number = number || !strcmp(c, DEC_KEY);

	return number;
}

bool GetLabel(char *c, string &label, int &replacementValue)
{
	//printf("c is %s\n", c);
	for (int i = 0; i < 10; i++)
	{
		//printf("checking label -- %s\n", labels[i].c_str());
		const char* lbl = labels[i].c_str();
		//printf("lbl %s\n", lbl);

		if (!strcmp(c, lbl))
		{ 
			label = labels[i];
			replacementValue = replacementValues[i];

			return true;
		}
	}

	return false;
}

void CalculateBase(char* tokens, int* base, char** arg)
{
	*base = -1;
	*arg = tokens;

	switch (tokens[0])
	{
		case BIN_KEY[0]:
			*base = 2;
			if (BIN_KEY != "") *arg = strtok(*arg, BIN_KEY);
			break;

		case HEX_KEY[0]:
			*base = 16;
			if (HEX_KEY != "") *arg = strtok(*arg, HEX_KEY);
			break;

		case DEC_KEY[0]:
			*base = 10;
			if (DEC_KEY != "") *arg = strtok(*arg, DEC_KEY);
			break;

		default:
			if (BIN_KEY == "") *base = 2;
			if (HEX_KEY == "") *base = 16;
			if (DEC_KEY == "") *base = 10;
			break;
	}
}