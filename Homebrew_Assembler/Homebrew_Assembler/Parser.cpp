#include "Parser.h"
#include <string>
#include <fstream>
#include <iostream>

void Parser::Parse(const char* filename)
{
	int retCode = -1;
	
	if (_outMode == OutMode::Verbose)
	{
		printf("\n -> Parsing: \"%s\" for %s\n", filename, _parseMode == ParseMode::Assembler ? "assembly" : "architecture configuration");
	}

	// Used to keep track of line that is currently being parsed
	int linenum = 0;
	string line;

	// Open the file for input
	ifstream inFile(filename);
	if (inFile.is_open())
	{
		// Initialize current token type
		_currTokenType = TokenType::None;

		// Pull in each line individually...will repeat until EOF
		while (getline(inFile, line))
		{
			// Skip any lines already processed
			if (_linePtr != -1 && linenum <= _linePtr && !_processingExternFile)
			{
				linenum++;
				continue;
			}

			// Line parse object needs to be reset for every line!
			ResetForNewLine();

			// Convert the line that was pulled in into an array of characters
			const char* c_line = line.c_str();

			printf("    -> Line #%d that is being parsed : \"%s\"\n", linenum + 1, c_line);

			if (c_line != NULL)
			{
				// Parse line into tokens
				ParseLineIntoTokens(c_line, " ,\t");

				if (_numTokens > 0)
				{
					char* label = NULL;
					TokenType tokenType = TokenType::None;
					for (int i = 0; i < _numTokens; i++)
					{
						if (_outMode == OutMode::Verbose)
						{
							// Echo current token to screen
							printf("       -> Token that is being parsed: #%d \"%s\"\n", i, _tokens[i]);
						}

						string currToken = _tokens[i];
						string nextToken;

						if (i + 1 < _numTokens)
							nextToken = _tokens[i + 1];

						// Parse token i
						retCode = ParseToken(i);

						if (retCode == -1) printf("ERROR occurred while parsing tokens\n");

						if (retCode == 1)
						{ 
							inFile.close();
							_processingExternFile = true;
							_currFile = strtok((char*)nextToken.c_str(), "\"");
							break;
						}

					}
				}
			}
			else
			{
				if (_outMode == OutMode::Verbose) printf("      -- Line not parsed because it was null.\n");
			}

			// Increase line number
			linenum++;
			_linePtr++;
		}

		if (_outMode == OutMode::Verbose && retCode != 1 && !_processingExternFile)
		{
			printf("\nDONE!\n\n\n");
		}

		if (retCode == 0 && !_processingExternFile) 
		{
			WriteToROM(filename);
		}

		// Once we reach EOF, close the file
		inFile.close();
	}
	else
	{
		printf("Unable to open file!\n");
	}

	// If ParseToken(i) returns a 1, that means we need to further process an architecture file
	if (retCode == 1)
	{
		int savedLinePtr = _linePtr;
		string filename_s = (string)_currFile;

		string preferredPath = "";
		string preferredExtension = "";

		if (_currTokenType == TokenType::Architecture)
		{ 
			preferredPath = "..\\Homebrew_Assembler\\Architecture_Config\\";
			preferredExtension = ".arch";
		}

		if (_currTokenType == TokenType::Include)
		{
			preferredPath = "..\\Homebrew_Assembler\\Assembly_Code\\";
			preferredExtension = ".asm";
		}

		string fullFile = SplitFilename(filename_s, preferredPath, preferredExtension, false);
		
		if (_currTokenType == TokenType::Architecture)
			ProcessRegisters(fullFile);
		else
			Parse(fullFile.c_str());
			
		_linePtr = savedLinePtr;
		_processingExternFile = false;
		_parseMode = ParseMode::Assembler;
		Parse(filename);
	}
}

void Parser::WriteToROM(const char* filename)
{
	_programROM.PrintList();
	_programROM.PrintTable();

	string filename_s = (string)filename;
	string preferredPath = "..\\Homebrew_Assembler\\ROM_Files\\";
	string preferredExtension = ".bin";

	string fullFile = SplitFilename(filename_s, preferredPath, preferredExtension, true);

	printf("Writing ROM data to %s\n", fullFile.c_str());
	_programROM.Write((char*)fullFile.c_str(), 32768);
}

void Parser::ProcessRegisters(string fullFile)
{
	// Open the architecture file for input
	ifstream inFile2(fullFile.c_str());
	if (inFile2.is_open())
	{
		// In a file, the first line number shows as 1 so set it to 1 for reporting line numbers in errors to the user
		int linenum = 0;
		string line;

		while (getline(inFile2, line))
		{
			printf("Reading Line #%d: %s\n", linenum, line.c_str());

			// Line parse object needs to be reset for every line!
			ResetForNewLine();

			// Convert the line that was pulled in into an array of characters
			const char* c_line = line.c_str();

			if (c_line != NULL)
			{
				// Parse line into tokens
				ParseLineIntoTokens(c_line, " ,\t");

				// Initialize some values we'll need for parsing registers and opcodes
				int cmdSize = -1;
				string ocmnemonic = "";
				bool ocEqualProcessed = false;
				bool opcodeIsAliased = false;

				if (_numTokens > 0)
				{
					// First token will indicate if the line is a register or an opcode
					if (!strcmp(_tokens[0], "register"))	_lineType = LineType::Register;
					if (!strcmp(_tokens[0], "opcode"))		_lineType = LineType::Opcode;
					if (!strcmp(_tokens[0], "opcode_alias"))
					{
						_lineType = LineType::Opcode;
						opcodeIsAliased = true;
					}

					// Second token will either be register size or opcode mnemonic
					cmdSize = stoi(_tokens[1], nullptr, 10);

					ocmnemonic = _lineType == LineType::Opcode ? _tokens[2] : "";
					_opcodeDictionary.currMnemonic = ocmnemonic;
					_opcodeDictionary.currNumArgs = 0;

					int iStart = _lineType == LineType::Register ? 2 : 3;

					if (_numTokens > 1)
					{
						// We've already processed the first two tokens...
						for (int i = iStart; i < _numTokens; i++)
						{
							printf("       -> Token that is being parsed: #%d \"%s\"\n", i, _tokens[i]);

							// If this is a register line, we need to add these registers to the register dictionary
							if (_lineType == LineType::Register)
							{
								// Skip parsing equal signs
								if (!strcmp(_tokens[i], "=")) continue;

								// If arch file was correctly typed, anything token at this point should be the label of a new register.
								// So, let's add it to the register dictionary. Throw an error if register has already been added.
								if (_registerDictionary.GetLabel((char*)_tokens[i]))
								{
									printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
									printf("  -> Register \"%s\" already defined! Parsing cannot continue until fixed\n", _tokens[i]);
									return;
								}
								else
								{
									// Add this register to the register dictionary
									_registerDictionary.Add(_tokens[i], cmdSize);
									printf("       -> Adding %d-bit register: \"%s\"\n", _registerDictionary.currValue, _registerDictionary.currLabel.c_str());
								}
							}

							if (_lineType == LineType::Opcode)
							{
								if (!strcmp(_tokens[i], "="))
								{
									ocEqualProcessed = true;
									continue;
								}

								// If we haven't yet read an equal sign, then the tokens correspond to the opcode definition.
								// If we have read the equal sign, then the token is the opcode value.
								if (!ocEqualProcessed)
								{
									// See if the argument is a register
									if (_registerDictionary.GetLabel(_tokens[i]))
									{
										if (_opcodeDictionary.currNumArgs == 0)
										{
											_opcodeDictionary.currArg0type = ArgType::Register;
											_opcodeDictionary.currArg0string = _tokens[i];
											_opcodeDictionary.currNumArgs++;
										}
										else
										{
											_opcodeDictionary.currArg1type = ArgType::Register;
											_opcodeDictionary.currArg1string = _tokens[i];
											_opcodeDictionary.currNumArgs++;
										}
									}
									else if (!strcmp(_tokens[i], "#"))
									{
										if (_opcodeDictionary.currNumArgs == 0)
										{
											_opcodeDictionary.currArg0type = ArgType::Numeral;
											_opcodeDictionary.currNumArgs++;
										}
										else
										{
											_opcodeDictionary.currArg1type = ArgType::Numeral;
											_opcodeDictionary.currNumArgs++;
										}
									}
									else
									{
										printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
										printf("  -> Unknown opcode argument \"%s\"! Parsing cannot continue until fixed\n", _tokens[i]);
										return;
									}
								}
								else
								{
									int base;
									char* arg;
									CalculateBase(i, &base, &arg);

									int ocval = stoi(arg, nullptr, base);

									if (_opcodeDictionary.GetOpcodeValue(ocval) && !opcodeIsAliased)
									{
										printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
										printf("  -> Value of \"%d\" assigned to opcode already exists! Parsing cannot continue until fixed\n", ocval);
										return;
									}

									_opcodeDictionary.currValue = ocval;
									_opcodeDictionary.currSize = cmdSize;

									int v;
									int s;
									if (_opcodeDictionary.currNumArgs == 0)
									{
										if (opcodeIsAliased || !_opcodeDictionary.Get0ArgOpcode(_opcodeDictionary.currMnemonic, &s, &v))
										{
											// Here's where the opcode is actually added to the dictionary
											_opcodeDictionary.AddCurrentEntry();
											printf("       -> Adding %d-bit opcode%s with pattern %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currValue);
										}
										else
										{
											printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
											printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
											return;
										}
									}

									if (_opcodeDictionary.currNumArgs == 1)
									{
										if (_opcodeDictionary.currArg0type == ArgType::Register)
										{
											if (opcodeIsAliased || !_opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, &s, &v))
											{
												// Here's where the opcode is actually added to the dictionary
												_opcodeDictionary.AddCurrentEntry();
												printf("       -> Adding %d-bit opcode%s with pattern %s %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currValue);
											}
											else
											{
												printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
												printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
												return;
											}
										}

										if (_opcodeDictionary.currArg0type == ArgType::Numeral)
										{
											if (opcodeIsAliased || !_opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, &s, &v))
											{
												// Here's where the opcode is actually added to the dictionary
												_opcodeDictionary.AddCurrentEntry();
												printf("       -> Adding %d-bit opcode%s with pattern %s # = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currValue);
											}
											else
											{
												printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
												printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
												return;
											}
										}
									}

									if (_opcodeDictionary.currNumArgs == 2)
									{
										if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Register)
										{
											if (opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1string, &s, &v))
											{
												// Here's where the opcode is actually added to the dictionary
												_opcodeDictionary.AddCurrentEntry();
												printf("       -> Adding %d-bit opcode%s with pattern %s %s, %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currArg1string.c_str(), _opcodeDictionary.currValue);
											}
											else
											{
												printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
												printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
												return;
											}
										}

										if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Numeral)
										{
											if (opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1num, &s, &v))
											{
												// Here's where the opcode is actually added to the dictionary
												_opcodeDictionary.AddCurrentEntry();
												printf("       -> Adding %d-bit opcode%s with pattern %s %s, # = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currValue);
											}
											else
											{
												printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
												printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
												return;
											}
										}

										if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.currArg1type == ArgType::Register)
										{
											if (opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, _opcodeDictionary.currArg1string, &s, &v))
											{
												// Here's where the opcode is actually added to the dictionary
												_opcodeDictionary.AddCurrentEntry();
												printf("       -> Adding %d-bit opcode%s with pattern %s #, %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg1string.c_str(), _opcodeDictionary.currValue);
											}
											else
											{
												printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _currFile.c_str());
												printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
												return;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			linenum++;
		}

		// Once we reach EOF, close the file
		inFile2.close();
	}
	else
	{
		printf("Could not open architecture file!\n");
	}
}

void Parser::ParseLineIntoTokens(const char* line, const char* delimiters)
{
	// Parse line into tokens by splitting on spaces, tabs, and commas
	// This command actually only performs one split...hence the while loop below
	char* tokens = strtok((char*)line, delimiters);
	_tokens.push_back(tokens);

	if (tokens == NULL)
	{
		_lineType = LineType::Blank;
	}
	else if (tokens != NULL && tokens[0] == ';')
	{
		_lineType = LineType::Comment;
	}
	else
	{
		switch (tokens[0])
		{
			case DIRECTIVE_KEYS[0]:
				_lineType = LineType::Directive;
				break;

			case SYMBOL_KEYS[0]:
				_lineType = LineType::Symbol;
				break;

			case LABEL_KEYS[0]:
				_lineType = LineType::Label;
				break;

			default:
				_lineType = LineType::OpCode;
				break;
		}

		// Continue splitting on the specified delimiters until none remain (i.e., NULL is returned)
		while (tokens != NULL)
		{
			// Print token out to the screen and perform another split
			_numTokens++;
			tokens = strtok(NULL, delimiters);

			_tokens.push_back(tokens);
		}
	}
}

int Parser::ParseToken(int i)
{
	if (i >= _numTokens) return -1;
	
	char* directive_parse = strtok(_tokens[i], DIRECTIVE_KEYS);
	char* symbol_parse = strtok(_tokens[i], SYMBOL_KEYS);
	char* label_parse = strtok(_tokens[i], LABEL_KEYS);

	if (i == 0)
	{
		if (!strcmp(directive_parse, ARCH_STR))
		{
			_currTokenType = TokenType::Architecture;
			return 1;
		}

		if (!strcmp(directive_parse, INCLUDE_STR) || !strcmp(directive_parse, INSERT_STR))
		{
			_currTokenType = TokenType::Include;
			return 1;
		}

		if (!strcmp(directive_parse, ORIGIN_STR))
		{
			_currTokenType = TokenType::Origin;
		}
	
		if (!strcmp(directive_parse, EXPORT_STR))
		{
			_currTokenType = TokenType::Export;
		}

		if (!strcmp(directive_parse, BYTE_STR))
		{
			_currTokenType = TokenType::Byte;
		}

		if (!strcmp(directive_parse, ASCII_STR))
		{
			_currTokenType = TokenType::Ascii;
		}

		if (strcmp(symbol_parse, _tokens[0]))
		{
			_currTokenType = TokenType::Symbol;
			_labelDictionary.currLabel = symbol_parse;
		}
	
		if (strcmp(label_parse, _tokens[0]))
		{
			_currTokenType = TokenType::Label;
			_labelDictionary.currLabel = label_parse;
		}

		if (_opcodeDictionary.IsAMnemonic(_tokens[0]))
		{
			_currTokenType = TokenType::OpCode;

			_opcodeDictionary.currMnemonic = _tokens[0];
			_opcodeDictionary.currNumArgs = 0;
		}
	}

	int base = -1;
	char* arg = _tokens[i];

	if (_currTokenType == TokenType::Origin && i == 1)
	{
		CalculateBase(1, &base, &arg);

		if (base != -1)
		{
			int address = stoi(arg, nullptr, base);
			printf("      -- Address set to: %02x\n", address);

			_programROM.SetCurrentAddress(address);

			_currTokenType = TokenType::None;
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for ORIGIN directive\n");
		}
	}

	if (_currTokenType == TokenType::Export && i > 0)
	{
		if (i == 1)
		{
			CalculateBase(1, &base, &arg);

			if (base != -1)
			{
				int startAddress = stoi(arg, nullptr, base);
				printf("      -- Start Address set to: %02x\n", startAddress);

				_programROM.SetStartAddress(startAddress);
			}
			else
			{
				printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
			}
		}

		if (i == 2)
		{
			CalculateBase(2, &base, &arg);

			if (base != -1)
			{
				int endAddress = stoi(arg, nullptr, base);
				printf("      -- End Address set to: %02x\n", endAddress);

				_programROM.SetEndAddress(endAddress);

				_currTokenType = TokenType::None;
			}
			else
			{
				printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
			}
		}
	}

	if (_currTokenType == TokenType::Byte && i > 0)
	{
		CalculateBase(i, &base, &arg);

		int byteVal = stoi(arg, nullptr, base);
		printf("      -- %02x: %02x\n", _programROM.GetCurrentAddress(), byteVal);

		_programROM.AddEntryToCurrentAddress(byteVal);
		_programROM.SetPattern("byte def");
		_programROM.IncrementCurrentAddress(1);
	}

	if (_currTokenType == TokenType::Ascii && i > 0)
	{
		char* a = strtok(_tokens[i], "\"");

		for (; *a; a++)
		{
			char currChar = *a != '/' ? *a : ' ';
			printf("      -- %02x: %c (%02x)\n", _programROM.GetCurrentAddress(), currChar, currChar);

			string charStr(1, currChar);

			_programROM.AddEntryToCurrentAddress((int)currChar);
			_programROM.SetPattern("ascii def: " + charStr);
			_programROM.IncrementCurrentAddress(1);
		}
	}

	if (_currTokenType == TokenType::Symbol && i == 1)
	{
		CalculateBase(1, &base, &arg);

		if (base != -1)
		{		
			_labelDictionary.currValue = stoi(arg, nullptr, base);
			_labelDictionary.AddCurrentEntry();

			printf("      -- Symbol: %s = %02x\n", _labelDictionary.currLabel.c_str(), _labelDictionary.currValue);

			_currTokenType = TokenType::None;
		}
	}

	if (_currTokenType == TokenType::Label)
	{
			_labelDictionary.currValue = _programROM.GetCurrentAddress();
			_labelDictionary.AddCurrentEntry();

			printf("      -- Label: %s = %02x\n", _labelDictionary.currLabel.c_str(), _labelDictionary.currValue);
						
			_currTokenType = TokenType::None;
	}

	if (_currTokenType == TokenType::OpCode)
	{
		if (i > 0)
		{
			if (i == 1)
			{
				CalculateBase(1, &base, &arg);

				if (IsNumeric(arg))
				{
					if (base != -1)
					{
						int arg0 = stoi(arg, nullptr, base);

						if (_opcodeDictionary.currNumArgs == 0)
						{
							_opcodeDictionary.currArg0type = ArgType::Numeral;
							_opcodeDictionary.currArg0num = arg0;
							_opcodeDictionary.currNumArgs++;
						}
					}
				}
				else
				{
					if (_registerDictionary.GetLabel(arg))
					{
						if (_opcodeDictionary.currNumArgs == 0)
						{
							_opcodeDictionary.currArg0type = ArgType::Register;
							_opcodeDictionary.currArg0string = arg;
							_opcodeDictionary.currNumArgs++;
						}
					}
					else
					{
						if (_labelDictionary.GetLabel(arg))
						{
							if (_opcodeDictionary.currNumArgs == 0)
							{
								_opcodeDictionary.currArg0type = ArgType::Numeral;
								_opcodeDictionary.currArg0num = _labelDictionary.currValue;
								_opcodeDictionary.currNumArgs++;
							}
						}
					}
				}
			}

			if (i == 2)
			{
				CalculateBase(2, &base, &arg);

				if (IsNumeric(arg))
				{
					if (base != -1)
					{
						int arg1 = stoi(arg, nullptr, base);

						if (_opcodeDictionary.currNumArgs > 0)
						{
							_opcodeDictionary.currArg1type = ArgType::Numeral;
							_opcodeDictionary.currArg1num = arg1;
							_opcodeDictionary.currNumArgs++;
						}
					}
				}
				else
				{
					if (_registerDictionary.GetLabel(arg))
					{
						if (_opcodeDictionary.currNumArgs > 0)
						{
							_opcodeDictionary.currArg1type = ArgType::Register;
							_opcodeDictionary.currArg1string = arg;
							_opcodeDictionary.currNumArgs++;
						}
					}
					else
					{
						if (_labelDictionary.GetLabel(arg))
						{
							if (_opcodeDictionary.currNumArgs > 0)
							{
								_opcodeDictionary.currArg1type = ArgType::Numeral;
								_opcodeDictionary.currArg1num = _labelDictionary.currValue;
								_opcodeDictionary.currNumArgs++;
							}
						}
					}

					_currTokenType = TokenType::None;
				}
			}
		}

		// TODO: Addresses are currently hard-coded to increase by 8...need to increment by actual size later!
		// --> opcodes are corrected!

		int oc_value = -1;
		int oc_size = -1;
		if (_opcodeDictionary.currNumArgs == 0)
		{
			if (_opcodeDictionary.Get0ArgOpcode(_opcodeDictionary.currMnemonic, &oc_size, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic);
				_programROM.IncrementCurrentAddress(oc_size/8);
			}
		}

		if (_opcodeDictionary.currNumArgs == 1)
		{
			if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, &oc_size, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " #");
				_programROM.IncrementCurrentAddress(oc_size/8);
				printf("      -- %02x: %02x\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg0num);
				_programROM.AddEntryToCurrentAddress(_opcodeDictionary.currArg0num);
				_programROM.SetPattern("#");
				_programROM.IncrementCurrentAddress(1);
			}

			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string, &oc_size, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " " + _opcodeDictionary.currArg0string);
				_programROM.IncrementCurrentAddress(oc_size/8);
			}
		}

		if (_opcodeDictionary.currNumArgs == 2)
		{
			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Register &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1string, &oc_size, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " " + _opcodeDictionary.currArg0string + ", " + _opcodeDictionary.currArg1string);
				_programROM.IncrementCurrentAddress(oc_size/8);
			}

			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Numeral &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1num, &oc_size, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " " + _opcodeDictionary.currArg0string + ", #");
				_programROM.IncrementCurrentAddress(oc_size/8);
				printf("      -- %02x: %02x\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg1num);
				_programROM.AddEntryToCurrentAddress(_opcodeDictionary.currArg1num);
				_programROM.SetPattern("#");
				_programROM.IncrementCurrentAddress(1);
			}

			if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.currArg1type == ArgType::Register &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, _opcodeDictionary.currArg1string, &oc_size, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " #, " + _opcodeDictionary.currArg1string);
				_programROM.IncrementCurrentAddress(oc_size/8);
				printf("      -- %02x: %02x\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg0num);
				_programROM.AddEntryToCurrentAddress(_opcodeDictionary.currArg0num);
				_programROM.SetPattern("#");
				_programROM.IncrementCurrentAddress(1);
			}
		}
	}

	return 0;
}

void Parser::CalculateBase(int i, int* base, char** arg)
{
	*base = -1;
	*arg = _tokens[i];

	switch (_tokens[i][0])
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

bool Parser::IsNumeric(const char* c)
{
	bool digit = false;

	for (; *c; c++)
	{
		if (*c == BIN_KEY[0] || *c == HEX_KEY[0] || *c == DEC_KEY[0])
			return true;

		// Test if equal to 0-9
		for (int j = 0; j < 10; j++)
		{
			char loopChar = to_string(j).c_str()[0];

			digit = *c == loopChar;
						
			if (digit) 
			{
				return true;
			}
		}
	}
	
	return false;
}

const string Parser::SplitFilename(const string& s, const string& preferredPath, const string& preferredExtension, bool forcePreferred)
{
	size_t last_slash = s.find_last_of("/\\");
	string path = s.substr(0, last_slash);
	string file_and_extension = s.substr(last_slash + 1);
	size_t extension_dot = file_and_extension.find_last_of(".");
	string file = file_and_extension.substr(0, extension_dot);
	string extension = "." + file_and_extension.substr(extension_dot + 1);

	if (last_slash == string::npos || forcePreferred)
		path = preferredPath;

	if (extension_dot == string::npos || forcePreferred)
		extension = preferredExtension;

	return path + file + extension;		
}