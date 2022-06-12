#include "Parser.h"
#include <string>
#include <fstream>
#include <iostream>

Parser::Parser()
{
	_opcodeDictionary = OpcodeDictionary();
	_registerDictionary = LabelDictionary(); 
	_labelDictionary = LabelDictionary();
	_archProcessed = false; 
	_archFile = ""; 
	_numTokens = 0; 
	_tokens.clear(); 
	_lineType = LineType::None;
	_outMode = OutMode::None; 
	_address = 0; 
	_startAddress = 0;
	_endAddress = 0; 
	_currTokenType = TokenType::None;
}

void Parser::Parse(char* filename)
{
	int retCode = -1;
	
	if (_outMode == OutMode::Verbose)
	{
		printf(" -> Parsing: \"%s\" for %s\n", filename, _parseMode == ParseMode::Assembler ? "assembly" : "architecture configuration");
	}

	// Used to keep track of line that is currently being parsed
	int linenum = 0;
	string line;

	// Open the file for input
	ifstream inFile(filename);
	if (inFile.is_open())
	{
		if (_parseMode == ParseMode::Assembler)
		{
			// Reset some variables
			_address = 0;
			_startAddress = 0;
			_endAddress = 0;
		//	_ocmnemonic = NULL;
		}

		// Initialize current token type
		_currTokenType = TokenType::None;

		// Pull in each line individually...will repeat until EOF
		while (getline(inFile, line))
		{
			// Line parse object needs to be reset for every line!
			ResetForNewLine();

			// Convert the line that was pulled in into an array of characters
			const char* c_line = line.c_str();

			if (c_line != NULL)
			{
				if (_outMode == OutMode::Verbose)
				{
					// Echo current line to screen
					printf("\n    -----------------------------------------------\n");
					printf("    -> parsing line #%d: \"%s\"\n", linenum, c_line);
					printf("    -----------------------------------------------\n");
				}

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
							//printf("       -> Token that is being parsed: #%d \"%s\"\n", i, _tokens[i]);
						}

						string currToken = _tokens[i];
						string nextToken;

						if (i + 1 < _numTokens)
							nextToken = _tokens[i + 1];

						// Parse token i
						retCode = ParseToken(i);

						if (retCode == 1)
							_archFile = strtok((char*)nextToken.c_str(), "\"");

						if (retCode == -1) printf("ERROR occurred while parsing tokens\n");
						if (retCode == 1 && !_archProcessed) { printf("close"); inFile.close(); break; }
					}
				}
				else
				{
					if (_outMode == OutMode::Verbose) printf("      -- No tokens were parsed.\n");
				}
			}
			else
			{
				if (_outMode == OutMode::Verbose) printf("      -- Line not parsed because it was null.\n");
			}

			// Increase line number
			linenum++;
		}

		if (_outMode == OutMode::Verbose && retCode != 1)
		{
			printf("\nDONE!\n");
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
		const char* archFile = _archFile.c_str();
		printf("opening %s for parsing syntax\n", archFile);
		
		// Open the architecture file for input
		ifstream inFile2(_archFile.c_str(), ifstream::in);
		if (inFile2.is_open())
		{
			// In a file, the first line number shows as 1 so set it to 1 for reporting line numbers in errors to the user
			linenum = 1;

			while (getline(inFile2, line))
			{
				//printf("Reading Line #%d: %s\n", linenum, line.c_str());

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
								//printf("       -> Token that is being parsed: #%d \"%s\"\n", i, _tokens[i]);

								// If this is a register line, we need to add these registers to the register dictionary
								if (_lineType == LineType::Register)
								{
									// Skip parsing equal signs
									if (!strcmp(_tokens[i], "=")) continue;

									// If arch file was correctly typed, anything token at this point should be the label of a new register.
									// So, let's add it to the register dictionary. Throw an error if register has already been added.
									if (_registerDictionary.GetLabel((char*)_tokens[i]))
									{
										printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
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
											printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
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
											printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
											printf("  -> Value of \"%d\" assigned to opcode already exists! Parsing cannot continue until fixed\n", ocval);
											return;
										}

										_opcodeDictionary.currValue = ocval;

										int v;
										if (_opcodeDictionary.currNumArgs == 0)
										{
											if (opcodeIsAliased || !_opcodeDictionary.Get0ArgOpcode(_opcodeDictionary.currMnemonic, &v))
											{
												// Here's where the opcode is actually added to the dictionary
												_opcodeDictionary.AddCurrentEntry();
												printf("       -> Adding %d-bit opcode%s with pattern %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currValue);
											}
											else
											{
												printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
												printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
												return;
											}
										}																			

										if (_opcodeDictionary.currNumArgs == 1)
										{
											if (_opcodeDictionary.currArg0type == ArgType::Register)
											{
												if (opcodeIsAliased || !_opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, &v))
												{
													// Here's where the opcode is actually added to the dictionary
													_opcodeDictionary.AddCurrentEntry();
													printf("       -> Adding %d-bit opcode%s with pattern %s %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currValue);
												}
												else
												{
													printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
													printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
													return;
												}
											}

											if (_opcodeDictionary.currArg0type == ArgType::Numeral)
											{
												if (opcodeIsAliased || !_opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, &v))
												{
													// Here's where the opcode is actually added to the dictionary
													_opcodeDictionary.AddCurrentEntry();
													printf("       -> Adding %d-bit opcode%s with pattern %s # = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currValue);
												}
												else
												{
													printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
													printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
													return;
												}
											}
										}

										if (_opcodeDictionary.currNumArgs == 2)
										{
											if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Register)
											{
												if (opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1string, &v))
												{
													// Here's where the opcode is actually added to the dictionary
													_opcodeDictionary.AddCurrentEntry();
													printf("       -> Adding %d-bit opcode%s with pattern %s %s, %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currArg1string.c_str(), _opcodeDictionary.currValue);
												}
												else
												{
													printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
													printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
													return;
												}
											}

											if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Numeral)
											{
												if (opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1num, &v))
												{
													// Here's where the opcode is actually added to the dictionary
													_opcodeDictionary.AddCurrentEntry();
													printf("       -> Adding %d-bit opcode%s with pattern %s %s, # = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currValue);
												}
												else
												{
													printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
													printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
													return;
												}
											}

											if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.currArg1type == ArgType::Register)
											{
												if (opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, _opcodeDictionary.currArg1string, &v))
												{
													// Here's where the opcode is actually added to the dictionary
													_opcodeDictionary.AddCurrentEntry();
													printf("       -> Adding %d-bit opcode%s with pattern %s #, %s = %02x\n", cmdSize, opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg1string.c_str(), _opcodeDictionary.currValue);
												}
												else
												{
													printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", linenum, _archFile.c_str());
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
		
		// Still need to assemble program, so do that now
		_numTokens = 0;
		_tokens.clear();

		_archProcessed = true;
		Parse(filename);
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
			case '*':
				if (!_archProcessed)
					_lineType = LineType::File;
				else
				{
					printf("      -- Arch File already processed -- Treating as a blank line!\n");
					_lineType = LineType::Blank;
				}

				break;

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

	if (!strcmp(directive_parse, ARCH_STR))
	{
		_currTokenType = TokenType::Architecture;

		return 1;
	}
	else if (!strcmp(directive_parse, ORIGIN_STR))
	{
		_currTokenType = TokenType::Origin;
	}
	else if (!strcmp(directive_parse, EXPORT_STR))
	{
		_currTokenType = TokenType::Export;
	}
	else if (!strcmp(directive_parse, BYTE_STR))
	{
		_currTokenType = TokenType::Byte;
	}
	else if (!strcmp(directive_parse, ASCII_STR))
	{
		_currTokenType = TokenType::Ascii;
	}
	else if (strcmp(symbol_parse, _tokens[i]))
	{
		_currTokenType = TokenType::Symbol;
		_labelDictionary.currLabel = symbol_parse;
	}
	else if (strcmp(label_parse, _tokens[i]))
	{
		_currTokenType = TokenType::Label;
		_labelDictionary.currLabel = label_parse;
	}

	// Only other token type to process is an OpCode!
	if (_lineType == LineType::OpCode)
	{
		if (_opcodeDictionary.IsAMnemonic(_tokens[i]))
		{
			_currTokenType = TokenType::OpCode;

			_opcodeDictionary.currMnemonic = _tokens[i];
			_opcodeDictionary.currNumArgs = 0;
		}
	}

	int base = -1;
	char* arg = _tokens[i];

	CalculateBase(i, &base, &arg);

	if (_currTokenType == TokenType::Origin && strcmp(directive_parse, ORIGIN_STR))
	{
		if (base != -1)
		{
			_address = stoi(arg, nullptr, base);
			printf("      -- Address set to: %02x\n", _address);

			_currTokenType = TokenType::None;
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for ORIGIN directive\n");
		}
	}

	if (_currTokenType == TokenType::Export && strcmp(directive_parse, EXPORT_STR))
	{
		if (base != -1)
		{
			_startAddress = stoi(arg, nullptr, base);
			printf("      -- Start Address set to: %02x\n", _startAddress);
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
		}

		CalculateBase(i + 1, &base, &arg);

		if (base != -1)
		{
			_endAddress = stoi(arg, nullptr, base);
			printf("      -- End Address set to: %02x\n", _endAddress);

			_currTokenType = TokenType::None;
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
		}
	}

	if (_currTokenType == TokenType::Byte && strcmp(directive_parse, BYTE_STR))
	{
		int byteVal = stoi(arg, nullptr, base);
		printf("      -- %02x: %02x\n", _address, byteVal);
		_address += 8;
	}

	if (_currTokenType == TokenType::Ascii && strcmp(directive_parse, ASCII_STR))
	{
		char* a = strtok(_tokens[i], "\"");

		for (; *a; a++)
		{
			char currChar = *a != '/' ? *a : ' ';
			printf("      -- %02x: %c (%02x)\n", _address, currChar, currChar);
			_address += 8;
		}
	}

	if (_currTokenType == TokenType::Symbol && !strcmp(symbol_parse, _tokens[i]))
	{
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
		if (base != -1)
		{
			_labelDictionary.currValue = _address;
			_labelDictionary.AddCurrentEntry();

			printf("      -- Label: %s = %02x\n", _labelDictionary.currLabel.c_str(), _labelDictionary.currValue);
						
			_currTokenType = TokenType::None;
		}
	}

	if (_currTokenType == TokenType::OpCode)
	{
		if (strcmp(arg, _opcodeDictionary.currMnemonic.c_str()))
		{
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

			if (i + 1 < _numTokens)
			{
				CalculateBase(i + 1, &base, &arg);

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

		int oc_value = -1;
		if (_opcodeDictionary.currNumArgs == 0)
		{

			if (_opcodeDictionary.Get0ArgOpcode(_opcodeDictionary.currMnemonic, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _address, oc_value, _opcodeDictionary.currMnemonic.c_str());
				_address += 8;
			}
		}

		if (_opcodeDictionary.currNumArgs == 1)
		{
			if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _address, oc_value, _opcodeDictionary.currMnemonic.c_str());
				_address += 8;
				printf("      -- %02x: %02x\n", _address, _opcodeDictionary.currArg0num);
				_address += 8;
			}

			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _address, oc_value, _opcodeDictionary.currMnemonic.c_str());
				_address += 8;
			}
		}

		if (_opcodeDictionary.currNumArgs == 2)
		{
			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Register &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1string, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _address, oc_value, _opcodeDictionary.currMnemonic.c_str());
				_address += 8;
			}

			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Numeral &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1num, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _address, oc_value, _opcodeDictionary.currMnemonic.c_str());
				_address += 8;
				printf("      -- %02x: %02x\n", _address, _opcodeDictionary.currArg1num);
				_address += 8;
			}

			if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.currArg1type == ArgType::Register &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, _opcodeDictionary.currArg1string, &oc_value))
			{
				printf("      -- %02x: %02x (%s)\n", _address, oc_value, _opcodeDictionary.currMnemonic.c_str());
				_address += 8;
				printf("      -- %02x: %02x\n", _address, _opcodeDictionary.currArg0num);
				_address += 8;
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

bool Parser::IsNumeric(char* c)
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
