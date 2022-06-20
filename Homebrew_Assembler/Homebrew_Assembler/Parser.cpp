#include "Parser.h"
#include <string>
#include <fstream>
#include <iostream>

/*=================================================== Parser::Parse ========================================================================
    DESCRIPTION: 
		  This function is the main one that takes in a filename and handles any needed processing of external files. It also calls the
		  function that tokenizes each line in the file and then calls another function that interprets each token.
===========================================================================================================================================*/
void Parser::Parse(const char* filename)
{
	// By default, initialize the return code from the ParseToken() function to be -1 (error state) so that we know
	// if for some reason this value wasn't set correctly.
	int retCode = -1;
	
	// Regardless of print mode, inform user what file we are processing
	if (_outMode == OutMode::Verbose || !_processingExternFile && _linePtr == -1)
		printf("\n -> Parsing: \"%s\" for %s\n", filename, _parseMode == ParseMode::Assembler ? "assembly" : "architecture configuration");

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
			// Skip any lines already processed -- this is needed because this function sets linenum to zero
			// every time it is called. We use _linePtr to save any previous line numbers in cases where 
			// another file needs to be parsed and we need to save the line number in the current file. So we
			// can skip any previously read lines so as not to process them multiple times by making sure that
			// we continue past the current iteration of the loop if linenum <= _linePtr.
			// 
			// If we are processing an external file, we actually want to start at line 0 so ignore _linePtr.
			// 
			// _linePtr will be -1 when this function is first called.
			if (_linePtr != -1 && linenum <= _linePtr && !_processingExternFile)
			{
				linenum++;
				continue;
			}

			// Line parse object needs to be reset for every line!
			ResetForNewLine();

			// Convert the line that was pulled in into an array of characters
			const char* c_line = line.c_str();

			if (_outMode == OutMode::Verbose)
				printf("    -> Line #%d that is being parsed : \"%s\"\n", linenum + 1, c_line);

			if (c_line != NULL)
			{
				// Tokenize the current line
				ParseLineIntoTokens(c_line, " ,\t");

				// If we actually have tokens...
				if (_numTokens > 0)
				{
					char* label = NULL;
					TokenType tokenType = TokenType::None;

					// Loop over all of them
					for (int i = 0; i < _numTokens; i++)
					{
						if (_outMode == OutMode::Verbose)
						{
							// Echo current token to screen
							printf("       -> Token that is being parsed: #%d \"%s\"\n", i, _tokens[i]);
						}

						// If we are not processing the last token, store the next token.
						// This is needed for directives whose second argument is an external file.
						// For example, if we have a directive like .arch "homewbrew.arch"...the file we want
						// is homebrew.arch
						string nextToken;
						if (i + 1 < _numTokens)
							nextToken = _tokens[i + 1];

						// Parse token i
						retCode = ParseToken(i);

						// If return code is -1 something has gone wrong
						if (retCode == -1) printf("ERROR occurred while parsing tokens\n");

						// If return code is 1, the parser has determined that we need to open and process another
						// external file before continuing with the current one.
						// For an example, look at demo.asm...you'll see the first line is .arch homebrew. When the
						// ParseToken() function processes the ".arch" directive, it returns a retCode of 1 because
						// it knows that homebrew.arch needs to be opened and processed before it can continue
						// parsing demo.asm. Further down in demo.asm, there's also a .insert test2.asm directive,
						// which again forces the parser to open test2.asm and process that before continuing with
						// parsing in demo.asm. From a practical standpoint, you can picture it like the homebrew.arch
						// and test2.asm files are copied and pasted in place of these lines. But this design was chosen
						// to make programs more manageable. If I write a bunch of OS file management code, for example,
						// I don't want to have to manually place that into every program that needs it. Instead, it would
						// be much nicer to write the single line .insert OSfileManager.asm.
						if (retCode == 1)
						{ 
							// Close the current file we are processing (don't worry...it will be opened again later)
							inFile.close();

							// Set a flag stating that we are processing an external file...this is mainly used to override
							// the line skipping behavior in the while loop above. If we are opening a new file, we always
							// want to start processing from line 0.
							_processingExternFile = true;

							// The directives that signal a new file needs to be parsed (i.e., those with return code 1)
							// all store the filename as their second token. strtok() is used here to chop off the 
							// quotation marks.
							_currFile = strtok((char*)nextToken.c_str(), "\"");
							break;
						}

					}
				}
			}
			else
			{
				if (_outMode == OutMode::Verbose) 
					printf("      -- Line not parsed because it was null.\n");
			}

			// Increase line number and line pointer.
			// Note that linenum always start at zero when this function is called. But if we come across any .arch, .include,
			// or .insert directives that require parsing another file, we need to save our current line number so that we can
			// return to it after we parse the new file. That's the purpose of linePtr. Note that it doesn't get reset to zero
			// like linenum does near the beginning of this function call.
			linenum++;
			_linePtr++;
		}

		// Only print done if we aren't processing an external file
		if (_outMode == OutMode::Verbose && retCode != 1 && !_processingExternFile)
		{
			printf("\nDONE!\n\n\n");
		}

		// If retCode is 0 and we aren't processing an external file at this point, then we've finished parsing the original
		// file and can write the program to ROM.
		if (retCode == 0 && !_processingExternFile) 
		{
			WriteProgramToROM(filename);
		}

		// Once we reach EOF, close the file
		inFile.close();
	}
	else
	{
		printf("Unable to open file!\n");
	}

	// If ParseToken(i) returns a 1, that means we need to further process an additional file
	if (retCode == 1)
	{
		// Save the current line pointer so we can resume parsing from that point after we parse this file 
		int savedLinePtr = _linePtr;

		// Convert the new filename to a string
		string filename_s = (string)_currFile;

		// Set the preferred path and extension in case the user didn't specify them in the program
		string preferredPath = "";
		string preferredExtension = "";
		if (_currTokenType == TokenType::Architecture)
		{ 
			_programROM.SetArchitecture(filename_s);
			preferredPath = "..\\Homebrew_Assembler\\Architecture_Config\\";
			preferredExtension = ".arch";
		}
		if (_currTokenType == TokenType::Include)
		{
			preferredPath = "..\\Homebrew_Assembler\\Assembly_Code\\";
			preferredExtension = ".asm";
		}

		// Build the full filepath string
		string fullFile = SplitFilename(filename_s, preferredPath, preferredExtension, false);
		
		// Parse this new file
		Parse(fullFile.c_str());
			
		// Since we are done parsing this new file, restore the saved line pointer so that we can continue parsing
		// from where we left off previously.
		_linePtr = savedLinePtr;

		// We are done processing this additional file, so set the flag to false and reset mode to assembler.
		_processingExternFile = false;
		_parseMode = ParseMode::Assembler;

		// Parse the original file, picking up where we left off previously
		Parse(filename);
	}
}

/*=============================================== Parser::WriteToROM() =====================================================================
	DESCRIPTION:
		  This function just prints a few versions of the interpreted program to the screen and writes the ROM data to a binary file.
===========================================================================================================================================*/
void Parser::WriteProgramToROM(const char* filename)
{
	// Setup the filepath string for the ROM file
	string filename_s = (string)filename;
	string preferredPath = "..\\Homebrew_Assembler\\ROM_Files\\";
	string preferredExtension = ".bin";
	string fullFile = SplitFilename(filename_s, preferredPath, preferredExtension, true);
	printf("\n\nWriting ROM data to %s\n", fullFile.c_str());

	// Print a list version of the interpreted program
	_programROM.PrintList();

	// Print a hex table of the data that will be written to the ROM
	_programROM.PrintTable();

	// Write the data to the ROM binary file
	_programROM.WriteProgram((char*)fullFile.c_str(), 32768);

	for (int i = 0; i < _controlROMs.size(); i++)
	{
		_controlROMs[i].WriteControlROM();
	}
}

/*============================================ Parser::ParseLineIntoTokens()================================================================
	DESCRIPTION:
		  This function just prints a few versions of the interpreted program to the screen and writes the ROM data to a binary file.
===========================================================================================================================================*/
void Parser::ParseLineIntoTokens(const char* line, const char* delimiters)
{
	// Parse line into tokens by splitting on spaces, tabs, and commas
	// This command actually only performs one split...hence the while loop below
	char* tokens = strtok((char*)line, delimiters);
	_tokens.push_back(tokens);

	_equalProcessed = false;
	_lastOperation = BinaryOperation::None;
	_ocValProcessed = false;
	_opcodeIsAliased = false;

	// Skip blank lines and comments. Else, determine the line type.
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

			// Only thing left is an opcode!
			default:
				_lineType = LineType::OpCode;
				break;
		}

		// Continue splitting on the specified delimiters until none remain (i.e., NULL is returned)
		while (tokens != NULL)
		{
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

		if (!strcmp(_tokens[0], REGISTER_STR))
		{
			_lineType = LineType::ArchRegister;
		}

		if (!strcmp(_tokens[0], CONTROL_STR))
		{
			_lineType = LineType::ArchControl;
		}

		if (!strcmp(_tokens[0], CONTROL_ALIAS_STR))
		{
			_lineType = LineType::ArchControlAlias;
			_lastOperation = BinaryOperation::None;
		}

		if (!strcmp(_tokens[0], OPCODE_STR))
		{
			_lineType = LineType::ArchOpcode;			
		}

		if (!strcmp(_tokens[0], OPCODE_ALIAS_STR))
		{
			_lineType = LineType::ArchOpcode;			
			_opcodeIsAliased = true;
		}

		if (!strcmp(_tokens[0], CONTROL_ROM_STR))
		{
			_lineType = LineType::ControlROM;
			_controlROMindex++;
			_controlROMs.push_back(ROMData());
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

			if (_outMode == OutMode::Verbose)
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

				if (_outMode == OutMode::Verbose)
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

				if (_outMode == OutMode::Verbose)
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

	if (_lineType == LineType::ArchRegister && i > 0)
	{
		int cmdSize = stoi(_tokens[1], nullptr, 10);

		// If arch file was correctly typed, anything token at this point should be the label of a new register.
		// So, let's add it to the register dictionary. Throw an error if register has already been added. Skip
		// parsing equal sign.
		if (strcmp(_tokens[i], "="))
		{
			if (!IsNumeric(_tokens[i]) && _registerDictionary.GetLabel((char*)_tokens[i]))
			{
				printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
				printf("  -> Register \"%s\" already defined! Parsing cannot continue until fixed\n", _tokens[i]);
				return -1;
			}
			else
			{
				// Add this register to the register dictionary
				_registerDictionary.Add(_tokens[i], cmdSize);

				if (_outMode == OutMode::Verbose)
					printf("       -> Adding %d-bit register: \"%s\"\n", _registerDictionary.currValue, _registerDictionary.currLabel.c_str());
			}
		}
	}

	if (_lineType == LineType::ArchControl && i > 0)
	{
		_controlDictionary.currLabel = _tokens[1];

		if (i > 1)
		{
			if (i >= 2)
			{
				if (strcmp(_tokens[i], "="))
				{
					int base;
					char* arg;
					CalculateBase(i, &base, &arg);

					int val = stoi(arg, nullptr, base);

					_controlDictionary.currValue = val;
					_controlDictionary.AddCurrentEntry();

					if (_outMode == OutMode::Verbose)
						printf("Adding Label %s with Value %08x\n", _controlDictionary.currLabel.c_str(), _controlDictionary.currValue);
				}
			}
		
		}
	}

	if (_lineType == LineType::ArchControlAlias && i > 0)
	{
		_controlDictionary.currLabel = _tokens[1];

		if (i > 1)
		{
			if (!strcmp(_tokens[i], "=") || !strcmp(_tokens[i], "{") || !strcmp(_tokens[i], "("))
			{
				_controlDictionary.currValue = 0;
				_lastOperation = BinaryOperation::None;

				if (_outMode == OutMode::Verbose)
					printf("     ---> ADDING CONTROL VAL: %08x\n", _controlDictionary.currValue);
			}
			else if (!strcmp(_tokens[i], "|"))
			{
				_lastOperation = BinaryOperation::LogicalOR;
			}
			else if (!strcmp(_tokens[i], "&"))
			{
				_lastOperation = BinaryOperation::LogicalAND;
			}
			else if (!strcmp(_tokens[i], "}") || !strcmp(_tokens[i], ")"))
			{
				_controlDictionary.AddCurrentEntry();

				if (_outMode == OutMode::Verbose)
					printf("Adding Label %s with Value %08x\n", _controlDictionary.currLabel.c_str(), _controlDictionary.currValue);
			}
			else
			{
				int val = _controlDictionary.GetLabelValue((char*)_tokens[i]);
				printf("         -> Val %s from dictionary: %08x\n", _tokens[i], val);
				if (val == -1)
				{
					int base;
					char* arg;
					CalculateBase(i, &base, &arg);

					val = stoi(arg, nullptr, base);
				}
								
				//if (_outMode == OutMode::Verbose)
					printf("         -> Val %s from dictionary: %08x\n", _tokens[i], val);

				switch (_lastOperation)
				{
				case BinaryOperation::LogicalOR:
					if (_outMode == OutMode::Verbose)
					{
						printf("     ---> OR operation\n");
						printf("     ---> Prior to op: %08x\n", _controlDictionary.currValue);
					}

					_controlDictionary.currValue = _controlDictionary.currValue | val;
					
					if (_outMode == OutMode::Verbose)
						printf("     ---> After op: %08x\n", _controlDictionary.currValue);
					break;

				case BinaryOperation::LogicalAND:
					if (_outMode == OutMode::Verbose)
					{
						printf("     ---> OR operation\n");
						printf("     ---> Prior to op: %08x\n", _controlDictionary.currValue);
					}

					_controlDictionary.currValue = _controlDictionary.currValue & val;

					if (_outMode == OutMode::Verbose)
						printf("     ---> After op: %08x\n", _controlDictionary.currValue);
					break;

				case BinaryOperation::None:
					_controlDictionary.currValue = val;
					if (_outMode == OutMode::Verbose)
						printf("     ---> ADDING VAL: %08X\n", _controlDictionary.currValue);
					break;
				}

				if (i == _numTokens - 1)
				{
					_controlDictionary.AddCurrentEntry();
					if (_outMode == OutMode::Verbose)
						printf("Adding Label %s with Value %08x\n", _controlDictionary.currLabel.c_str(), _controlDictionary.currValue);
				}
			}
		}
	}

	if (_lineType == LineType::ArchOpcode && i > 0)
	{
		int cmdSize = stoi(_tokens[1], nullptr, 10);

		_opcodeDictionary.currMnemonic = _tokens[2];

		if (i == 1)
			_opcodeDictionary.currNumArgs = 0;

		if (!strcmp(_tokens[i], "="))
		{
			_equalProcessed = true;
		}

		// If we haven't yet read an equal sign, then the tokens correspond to the opcode definition.
		// If we have read the equal sign, then the token is the opcode value or control line specification.
		if (!_equalProcessed && i > 2 && strcmp(_tokens[i], "="))
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
				printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
				printf("  -> Unknown opcode argument \"%s\"! Parsing cannot continue until fixed\n", _tokens[i]);
				return -1;
			}
		}
		else if (_equalProcessed && strcmp(_tokens[i], "="))
		{
			if (!_ocValProcessed)
			{
				int base;
				char* arg;
				CalculateBase(i, &base, &arg);

				int ocval = stoi(arg, nullptr, base);

				if (_opcodeDictionary.GetOpcodeValue(ocval) && !_opcodeIsAliased)
				{
					printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
					printf("  -> Value of \"%d\" assigned to opcode already exists! Parsing cannot continue until fixed\n", ocval);
					return -1;
				}

				_opcodeDictionary.currValue = ocval;
				_opcodeDictionary.currSize = cmdSize;

				_ocValProcessed = true;
			}
			else
			{
				// TODO: Make sure the process control line pattern is correct here
				if (!strcmp(_tokens[i], "{") || !strcmp(_tokens[i], "("))
				{
					_opcodeDictionary.currControlPattern = 0;
					_lastOperation = BinaryOperation::None;

					if (_outMode == OutMode::Verbose)
						printf("     ---> Initializing Control Pattern: %08X\n", _opcodeDictionary.currControlPattern);
				}
				else if (!strcmp(_tokens[i], "|"))
				{
					_lastOperation = BinaryOperation::LogicalOR;
				}
				else if (!strcmp(_tokens[i], "&"))
				{
					_lastOperation = BinaryOperation::LogicalAND;
				}
				else if (!strcmp(_tokens[i], "}") || !strcmp(_tokens[i], ")"))
				{
					int v;
					int s;
					int cp;
					if (_opcodeDictionary.currNumArgs == 0)
					{
						if (_opcodeIsAliased || !_opcodeDictionary.Get0ArgOpcode(_opcodeDictionary.currMnemonic, &s, &v, &cp))
						{
							// Here's where the opcode is actually added to the dictionary
							_opcodeDictionary.AddCurrentEntry();
							printf("       -> Adding %d-bit opcode%s with pattern %s = %02x using control line sequence %08x\n", cmdSize, _opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currValue, _opcodeDictionary.currControlPattern);
						}
						else
						{
							printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
							printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
							return -1;
						}
					}

					if (_opcodeDictionary.currNumArgs == 1)
					{
						if (_opcodeDictionary.currArg0type == ArgType::Register)
						{
							if (_opcodeIsAliased || !_opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, &s, &v, &cp))
							{
								// Here's where the opcode is actually added to the dictionary
								_opcodeDictionary.AddCurrentEntry();
								printf("       -> Adding %d-bit opcode%s with pattern %s %s = %02x using control line sequence %08x\n", cmdSize, _opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currValue, _opcodeDictionary.currControlPattern);
							}
							else
							{
								printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
								printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
								return -1;
							}
						}

						if (_opcodeDictionary.currArg0type == ArgType::Numeral)
						{
							if (_opcodeIsAliased || !_opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, &s, &v, &cp))
							{
								// Here's where the opcode is actually added to the dictionary
								_opcodeDictionary.AddCurrentEntry();
								printf("       -> Adding %d-bit opcode%s with pattern %s # = %02x using control line sequence %08x\n", cmdSize, _opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currValue, _opcodeDictionary.currControlPattern);
							}
							else
							{
								printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
								printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
								return -1;
							}
						}
					}

					if (_opcodeDictionary.currNumArgs == 2)
					{
						if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Register)
						{
							if (_opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1string, &s, &v, &cp))
							{
								// Here's where the opcode is actually added to the dictionary
								_opcodeDictionary.AddCurrentEntry();
								printf("       -> Adding %d-bit opcode%s with pattern %s %s, %s = %02x using control line sequence %08x\n", cmdSize, _opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currArg1string.c_str(), _opcodeDictionary.currValue, _opcodeDictionary.currControlPattern);
							}
							else
							{
								printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
								printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
								return -1;
							}
						}

						if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Numeral)
						{
							if (_opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1num, &s, &v, &cp))
							{
								// Here's where the opcode is actually added to the dictionary
								_opcodeDictionary.AddCurrentEntry();
								printf("       -> Adding %d-bit opcode%s with pattern %s %s, # = %02x using control line sequence %08x\n", cmdSize, _opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string.c_str(), _opcodeDictionary.currValue, _opcodeDictionary.currControlPattern);
							}
							else
							{
								printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
								printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
								return -1;
							}
						}

						if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.currArg1type == ArgType::Register)
						{
							if (_opcodeIsAliased || !_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, _opcodeDictionary.currArg1string, &s, &v, &cp))
							{
								// Here's where the opcode is actually added to the dictionary
								_opcodeDictionary.AddCurrentEntry();
								printf("       -> Adding %d-bit opcode%s with pattern %s #, %s = %02x using control line sequence %08x\n", cmdSize, _opcodeIsAliased ? "-alias" : "", _opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg1string.c_str(), _opcodeDictionary.currValue, _opcodeDictionary.currControlPattern);
							}
							else
							{
								printf("\n\n!!! CRITICAL ERROR in Line #%d of \"%s\" !!!\n", _linePtr + 1, _currFile.c_str());
								printf("  -> Opcode pattern already exists! Parsing cannot continue until fixed\n");
								return -1;
							}
						}
					}
				}
				else
				{
					// Pull value from the control dictionary
					int val = _controlDictionary.GetLabelValue(_tokens[i]);				

					// If not found in the control dictionary, it might be a numeric value
					if (val == -1 && IsNumeric(_tokens[i]))
					{
						int base;
						char* arg;
						CalculateBase(i, &base, &arg);

						val = stoi(arg, nullptr, base);
					}

					//if (_outMode == OutMode::Verbose)
						printf("         -> Val %s from dictionary: %08x\n", _tokens[i], val);

					switch (_lastOperation)
					{
					case BinaryOperation::LogicalOR:
						if (_outMode == OutMode::Verbose)
						{
							printf("     ---> OR operation\n");
							printf("     ---> Prior to op: %08x\n", _opcodeDictionary.currControlPattern);
						}

						_opcodeDictionary.currControlPattern = _opcodeDictionary.currControlPattern | val;
						if (_outMode == OutMode::Verbose)
							printf("     ---> After op: %08x\n", _opcodeDictionary.currControlPattern);
						break;

					case BinaryOperation::LogicalAND:
						if (_outMode == OutMode::Verbose)
						{
							printf("     ---> OR operation\n");
							printf("     ---> Prior to op: %08x\n", _opcodeDictionary.currControlPattern);
						}
						_opcodeDictionary.currControlPattern = _opcodeDictionary.currControlPattern & val;
						if (_outMode == OutMode::Verbose)
						{
							printf("     ---> After op: %08x\n", _opcodeDictionary.currControlPattern);
						}

						break;

					case BinaryOperation::None:
						_opcodeDictionary.currControlPattern = val;

						if (_outMode == OutMode::Verbose)
							printf("     ---> ADDING VAL: %08X\n", _opcodeDictionary.currControlPattern);
						break;
					}
				}				
			}
		}
	}

	if (_lineType == LineType::ControlROM && i > 0)
	{
		if (i == 1)
		{
			int val = stoi(_tokens[i]);
			_controlROMs[_controlROMindex].SetBitWidth(val);
		}

		if (i == 2)
		{
			int val = stoi(_tokens[i]);
			_controlROMs[_controlROMindex].SetROMsize(val);
		}

		if (i == 3)
		{
			_controlROMs[_controlROMindex].SetROMname(string(_tokens[i],strlen(_tokens[i])));
		}
	}

	if (_currTokenType == TokenType::Byte && i > 0)
	{
		CalculateBase(i, &base, &arg);

		int byteVal = stoi(arg, nullptr, base);

		if (_outMode == OutMode::Verbose)
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

			if (_outMode == OutMode::Verbose)
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

			if (_outMode == OutMode::Verbose)
				printf("      -- Symbol: %s = %02x\n", _labelDictionary.currLabel.c_str(), _labelDictionary.currValue);

			_currTokenType = TokenType::None;
		}
	}

	if (_currTokenType == TokenType::Label)
	{
			_labelDictionary.currValue = _programROM.GetCurrentAddress();
			_labelDictionary.AddCurrentEntry();

			if (_outMode == OutMode::Verbose)
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
					else if (_labelDictionary.GetLabel(arg))
					{
						if (_opcodeDictionary.currNumArgs == 0)
						{
							_opcodeDictionary.currArg0type = ArgType::Numeral;
							_opcodeDictionary.currArg0num = _labelDictionary.currValue;
							_opcodeDictionary.currNumArgs++;
						}						
					}
					else
					{
						if ((unsigned char)_tokens[i] <= 127)
						{
							char* a = strtok(_tokens[i], "\"");

							for (; *a; a++)
							{
								char currChar = *a != '/' ? *a : ' ';

							
									printf("      -- %02x: %c (%02x)\n", _programROM.GetCurrentAddress(), currChar, currChar);
							}

							_opcodeDictionary.currArg0type = ArgType::Ascii;
							_opcodeDictionary.currArg0num = (int)_tokens[i];
							_opcodeDictionary.currNumArgs++;
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
					else if (_labelDictionary.GetLabel(arg))
					{
						if (_opcodeDictionary.currNumArgs > 0)
						{
							_opcodeDictionary.currArg1type = ArgType::Numeral;
							_opcodeDictionary.currArg1num = _labelDictionary.currValue;
							_opcodeDictionary.currNumArgs++;
						}		
					}
					else
					{
						char* a = strtok(_tokens[i], "\"");

						char currChar;
						for (; *a; a++)
						{
							currChar = *a != '/' ? *a : ' ';


							printf("      -- %02x: %c (%02x)\n", _programROM.GetCurrentAddress(), currChar, currChar);
						}

							_opcodeDictionary.currArg1type = ArgType::Ascii;
							_opcodeDictionary.currArg1num = (int)currChar;
							_opcodeDictionary.currNumArgs++;

					}

					_currTokenType = TokenType::None;
				}
			}
		}

		// TODO: Addresses are currently hard-coded to increase by 8...need to increment by actual size later!
		// --> opcodes are corrected!

		int oc_value = -1;
		int oc_size = -1;
		int oc_ctrlPattern = -1;
		if (_opcodeDictionary.currNumArgs == 0)
		{
			if (_opcodeDictionary.Get0ArgOpcode(_opcodeDictionary.currMnemonic, &oc_size, &oc_value, &oc_ctrlPattern))
			{
				if (_outMode == OutMode::Verbose)
					printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic);
				_programROM.IncrementCurrentAddress(oc_size/8);
			}
		}

		if (_opcodeDictionary.currNumArgs == 1)
		{
			if ((_opcodeDictionary.currArg0type == ArgType::Numeral || _opcodeDictionary.currArg0type == ArgType::Ascii) && _opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, &oc_size, &oc_value, &oc_ctrlPattern))
			{
				if (_outMode == OutMode::Verbose)
					printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " #");
				_programROM.IncrementCurrentAddress(oc_size/8);
				if (_outMode == OutMode::Verbose)
				{
					if (_opcodeDictionary.currArg0type == ArgType::Numeral)
						printf("      -- %02x: %02x\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg0num);
					else
						printf("      -- %02x: %c\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg0num);
				}
				_programROM.AddEntryToCurrentAddress(_opcodeDictionary.currArg0num);				
				
				char c = static_cast<char>(_opcodeDictionary.currArg0num);
				printf("Char is %c\n", c);
				string s(1, c);

				if (_opcodeDictionary.currArg0type == ArgType::Numeral)
					_programROM.SetPattern("#");
				else
					_programROM.SetPattern(s);
				_programROM.IncrementCurrentAddress(1);
			}

			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.Get1ArgOpcode(_opcodeDictionary.currMnemonic.c_str(), _opcodeDictionary.currArg0string, &oc_size, &oc_value, &oc_ctrlPattern))
			{
				if (_outMode == OutMode::Verbose)
					printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " " + _opcodeDictionary.currArg0string);
				_programROM.IncrementCurrentAddress(oc_size/8);
			}
		}

		if (_opcodeDictionary.currNumArgs == 2)
		{
			if (_opcodeDictionary.currArg0type == ArgType::Register && _opcodeDictionary.currArg1type == ArgType::Register &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1string, &oc_size, &oc_value, &oc_ctrlPattern))
			{
				if (_outMode == OutMode::Verbose)
					printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " " + _opcodeDictionary.currArg0string + ", " + _opcodeDictionary.currArg1string);
				_programROM.IncrementCurrentAddress(oc_size/8);
			}

			if (_opcodeDictionary.currArg0type == ArgType::Register && (_opcodeDictionary.currArg1type == ArgType::Numeral || _opcodeDictionary.currArg1type == ArgType::Ascii) &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0string, _opcodeDictionary.currArg1num, &oc_size, &oc_value, &oc_ctrlPattern))
			{
				if (_outMode == OutMode::Verbose)
					printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " " + _opcodeDictionary.currArg0string + ", #");
				_programROM.IncrementCurrentAddress(oc_size/8);
				if (_outMode == OutMode::Verbose)
				{
					if (_opcodeDictionary.currArg1type == ArgType::Numeral)
						printf("      -- %02x: %02x\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg1num);
					else
						printf("      -- %02x: %c\n", _programROM.GetCurrentAddress(), _opcodeDictionary.currArg1num);
				}
				_programROM.AddEntryToCurrentAddress(_opcodeDictionary.currArg1num);
				char c = static_cast<char>(_opcodeDictionary.currArg1num);
				printf("Char is %c\n", c);
				string s(1, c);

				if (_opcodeDictionary.currArg1type == ArgType::Numeral)
					_programROM.SetPattern("#");
				else
					_programROM.SetPattern(s);
				_programROM.IncrementCurrentAddress(1);
			}

			if (_opcodeDictionary.currArg0type == ArgType::Numeral && _opcodeDictionary.currArg1type == ArgType::Register &&
				_opcodeDictionary.Get2ArgOpcode(_opcodeDictionary.currMnemonic, _opcodeDictionary.currArg0num, _opcodeDictionary.currArg1string, &oc_size, &oc_value, &oc_ctrlPattern))
			{
				if (_outMode == OutMode::Verbose)
					printf("      -- %02x: %02x (%s)\n", _programROM.GetCurrentAddress(), oc_value, _opcodeDictionary.currMnemonic.c_str());
				_programROM.AddEntryToCurrentAddress(oc_value);
				_programROM.SetPattern(_opcodeDictionary.currMnemonic + " #, " + _opcodeDictionary.currArg1string);
				_programROM.IncrementCurrentAddress(oc_size/8);
				if (_outMode == OutMode::Verbose)
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

/*================================================== Parser::IsNumeric()================================================================
	DESCRIPTION:
		  This is a helper function which determines if the provided character matches 0-9 or any numeric prefixes.
===========================================================================================================================================*/
bool Parser::IsNumeric(const char* c)
{
	bool digit = false;

	for (; *c; c++)
	{
		// Test prefixes
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

/*================================================== Parser::SplitFilename()================================================================
	DESCRIPTION:
		  This is a simple helper function that splits up the path, filename, and extension. If no path or extension are provided, or the 
		  forcePreferred boolean is true, the resulting full path is constructed with the preferred path and preferred extension.
===========================================================================================================================================*/
const string Parser::SplitFilename(const string& s, const string& preferredPath, const string& preferredExtension, bool forcePreferred)
{
	// Find the last slash to get the end of the path and build the path string
	size_t last_slash = s.find_last_of("/\\");
	string path = s.substr(0, last_slash);

	// The file and extension should be everything beyond the last slash
	string file_and_extension = s.substr(last_slash + 1);

	// Find the last dot, which should be the separator between file and extension. Build the path and extension strings with this knowledge.
	size_t extension_dot = file_and_extension.find_last_of(".");
	string file = file_and_extension.substr(0, extension_dot);
	string extension = "." + file_and_extension.substr(extension_dot + 1);

	// If no slash is found or we want to force the preferred path, then set the path to the preferred path
	if (last_slash == string::npos || forcePreferred)
		path = preferredPath;

	// If no dot is found or we want to force the preferred extension, then set the extension to the preferred extension
	if (extension_dot == string::npos || forcePreferred)
		extension = preferredExtension;

	// Return the full filepath
	return path + file + extension;		
}