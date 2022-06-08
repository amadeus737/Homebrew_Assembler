#include <string>
#include <fstream>
#include "Parser.h"
#include "Config.h"

void Parser::Parse(char* filename)
{
	if (_mode == Mode::Verbose)
	{
		printf(" -> Parsing file: \"%s\"\n", filename);
	}

	// Used to keep track of line that is currently being parsed
	int linenum = 0;

	// Holds currently parsed line
	string line;

	// Open the file for input
	ifstream inFile(filename);
	if (inFile.is_open())
	{
		// Reset some variables
		_address = 0;
		_startAddress = 0;
		_endAddress = 0;
		_ocmnemonic = NULL;
		_currTokenType = TokenType::None;

		// Being phase 0 of parsing (file -> lines)
		// Pull in each line individually...will repeat until EOF
		while (getline(inFile, line))
		{
			// Line parse object needs to be reset for every line!
			Reset();

			// Convert the line that was pulled in into an array of characters
			const char* c_line = line.c_str();

			if (c_line != NULL)
			{
				if (_mode == Mode::Verbose)
				{
					// Echo current line to screen
					printf("\n    ------------------------------------------\n");
					printf("    -> parsing line #%d \"%s\"\n", linenum, c_line);
					printf("    ------------------------------------------\n");
				}

				// Phase 1 of parsing (line -> tokens)
				ParseLineIntoTokens(c_line, " ,\t");
				
				// If there are actually tokens to parse...
				if (_numTokens > 0)
				{
					// Loop over all tokens in the line
					for (int i = 0; i < _numTokens; i++)
					{
						if (_mode == Mode::Verbose)
						{
							// Echo current token to screen
							//	printf("       -> Token that is being parsed: #%d \"%s\"\n", i, _tokens[i]);
						}

						// Parse token i
						ParseToken(i);
					}
				}
				else
				{
					if (_mode == Mode::Verbose) printf("      -- No tokens were parsed.\n");
				}
			}
			else
			{
				if (_mode == Mode::Verbose) printf("      -- Line not parsed because it was null.\n");
			}

			// Increase line number
			linenum++;
		}

		if (_mode == Mode::Verbose)
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
}

void Parser::ParseLineIntoTokens(const char* line, const char* delimiters)
{
	// Parse line into tokens by splitting on spaces, tabs, and commas
	char* tokens = strtok((char*)line, delimiters);

	// Add the parsed tokens to _tokens vector
	_tokens.push_back(tokens);

	// Handle blank and comment lines
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
		// Handle directive, symbol, label, and opcode tokens
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
			// Perform another split
			_numTokens++;
			tokens = strtok(NULL, delimiters);

			// Add to _tokens vector
			_tokens.push_back(tokens);
		}
	}
}

void Parser::ParseToken(int i)
{
	if (i >= _numTokens) return;

	// Test-parse each token with DIRECTIVE_KEYS, SYMBOL_KEYS, and LABEL_KEYS
	char* directive_parse = strtok(_tokens[i], DIRECTIVE_KEYS);
	char* symbol_parse = strtok(_tokens[i], SYMBOL_KEYS);
	char* label_parse = strtok(_tokens[i], LABEL_KEYS);

	// Note: strcmp is a little confusing because it actually returns an integral value. 0 is returned if the strings
	// match, which would be interpreted as a logical false in an if statement. For that reason, !strcmp() is typically
	// used where you would probably expect to see just strcmp().

	if (!strcmp(directive_parse, ORIGIN_STR))
	{
		// If the test-parse of the token with the DIRECTIVE_KEYS results in ORIGIN_STR, set _currTokenType to origin
		// so that it can be further processed
		_currTokenType = TokenType::Origin;
	}
	else if (!strcmp(directive_parse, EXPORT_STR))
	{
		// If the test-parse of the token with the DIRECTIVE_KEYS results in EXPORT_STR, set _currTokenType to export
		// so that it can be further processed
		_currTokenType = TokenType::Export;
	}
	else if (strcmp(symbol_parse, _tokens[i]))
	{
		// If the test-parse of the token with the SYMBOL_KEYS does not match the pre-parsed string (_tokens[i]), then
		// that means it was successfully parsed
		_currTokenType = TokenType::Symbol;

		// Store the parsed symbol in the label dictionary
		_labelCount++;
		_label = symbol_parse;
		_labels[_labelCount] = _label;
	}
	else if (strcmp(label_parse, _tokens[i]))
	{
		// If the test-parse of the token with the LABEL_KEYS does not match the pre-parsed string (_tokens[i]), then
		// that means it was successfully parsed
		_currTokenType = TokenType::Label;

		// Store the parsed label in the label dictionary
		_labelCount++;
		_label = label_parse;
		_labels[_labelCount] = _label;
	}

	if (_lineType == LineType::OpCode)
	{
		// If the current token being parsed is an opcode mnemonic...
		if (IsAMnemonic(_tokens[i]))
		{
			// Store the current token type as opcode for further processing
			_currTokenType = TokenType::OpCode;

			// Store the opcode mnemonic and set the new opcode flag
			_ocmnemonic = _tokens[i];
			_newOpCode = true;

			// Print the address of this opcode mnemonic
			printf("      -- %02x : %s\n", _address, _ocmnemonic);

			// Later, replace with actual size of opcode
			// For now, just assume each opcode is 8 bits wide
			_address += 8;
		}
	}

	// Calculate base and return modified token
	int base = -1;
	char* arg = _tokens[i];
	CalculateBase(i, &base, &arg);

	// If the current token type is origin directive and directive_parse is NOT ORIGIN_STR, then we 
	// are parsing the argument of the origin directive
	if (_currTokenType == TokenType::Origin && strcmp(directive_parse, ORIGIN_STR))
	{
		// If we have a valid base for a numerical argument
		if (base != -1)
		{
			// Convert the parsed token to an address based on the base
			_address = stoi(arg, nullptr, base);
			printf("      -- Address set to: %x\n", _address);

			// Clear current token type since we are done processing a complete origin directive command
			_currTokenType = TokenType::None;
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for ORIGIN directive\n");
		}
	}

	// If the current token type is export directive and directive_parse is NOT EXPORT_STR, then we 
	// are parsing the arguments of the export directive
	if (_currTokenType == TokenType::Export && strcmp(directive_parse, EXPORT_STR))
	{
		// If we have a valid base for a numerical argument
		if (base != -1)
		{
			// Convert the parsed token to a start export address based on the base
			_startAddress = stoi(arg, nullptr, base);
			printf("      -- Start Address set to: %x\n", _startAddress);
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
		}

		// Calculate base and return modified token for next token (i+1)
		CalculateBase(i + 1, &base, &arg);

		if (base != -1)
		{
			// Convert the parsed token to a end export address based on the base
			_endAddress = stoi(arg, nullptr, base);
			printf("      -- End Address set to: %x\n", _endAddress);

			// Clear current token type since we are done processing a complete export directive command
			_currTokenType = TokenType::None;
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
		}
	}

	// If the current token type is symbol and symbol_parse is NOT equal to the pre-parsed token, then we
	// are parsing the arguments of the symbol
	if (_currTokenType == TokenType::Symbol && !strcmp(symbol_parse, _tokens[i]))
	{
		// If we have a valid numerical base
		if (base != -1)
		{
			// Convert the parsed token (arg) to a replacement value based on the numerical base
			int replacementValue = stoi(arg, nullptr, base);
			printf("      -- Symbol: %s = %x\n", _label.c_str(), replacementValue);

			// Store the replacement value in the replacement value vector
			_replacementValues[_labelCount] = replacementValue;

			// Clear current token type since we are done processing a complete symbol command
			_currTokenType = TokenType::None;
		}
	}

	// If the current token type is label (no arguments)
	if (_currTokenType == TokenType::Label)
	{
		if (base != -1)
		{
			// Save address to label
			int replacementValue = _address;
			printf("      -- Label: %s = %x\n", _label.c_str(), replacementValue);

			// Store address in replacement value vector
			_replacementValues[_labelCount] = replacementValue;

			// Clear current token type since we are done processing a complete label command
			_currTokenType = TokenType::None;
		}
	}

	// Handle opcodes
	if (_currTokenType == TokenType::OpCode)
	{
		// If we are parsing an opcode argument, arg != _ocmnemonic
		if (strcmp(arg, _ocmnemonic) && _newOpCode)
		{
			// New opcode flag meant to only trigger this code block once
			_newOpCode = false;

			if (i < _numTokens)
			{
				// If the argument is a number
				if (IsNumeric(arg))
				{
					if (base != -1)
					{
						// Convert parsed token to integer via the base
						int arg0 = stoi(arg, nullptr, base);
						printf("      -- %02x : %02x\n", _address, arg0);

						// Increment address by 8 bytes (change later if needed)
						_address += 8;
					}
				}
				else
				{
					// If the argument is a label in the label dictionary...
					if (GetLabel(arg))
					{
						printf("      -- %02x : %02x\n", _address, _replacementValue);

						// Increment address by 8 bytes (change later if needed)
						_address += 8;
					}
				}

				if (i + 1 < _numTokens)
				{
					// Process next argument, if present
					CalculateBase(i + 1, &base, &arg);

					// Handle numerical argument
					if (IsNumeric(arg))
					{
						if (base != -1)
						{
							// Convert parsed token to integer via the base
							int arg1 = stoi(arg, nullptr, base);
							printf("      -- %02x : %02x\n", _address, arg1);

							// Increment address by 8 bytes (change later if needed)
							_address += 8;
						}
					}
					else
					{
						// If the argument is a label in the label dictionary...
						if (GetLabel(arg))
						{
							printf("      -- %02x : %02x\n", _address, _replacementValue);

							// Increment address by 8 bytes (change later if needed)
							_address += 8;

							// Clear token type since we've processed a complete opcode command
							_currTokenType = TokenType::None;
						}
					}
				}
			}
		}
	}
}

void Parser::CalculateBase(int i, int* base, char** arg)
{
	// Initialize return values
	*base = -1;
	*arg = _tokens[i];

	// Test first character of token i
	switch (_tokens[i][0])
	{
		// If it is equal to the binary key..
		case BIN_KEY[0]:
			*base = 2;
			if (BIN_KEY != "") *arg = strtok(*arg, BIN_KEY);
			break;

		// If it is equal to the hex key...
		case HEX_KEY[0]:
			*base = 16;
			if (HEX_KEY != "") *arg = strtok(*arg, HEX_KEY);
			break;

		// If it is equal to the decimal key...
		case DEC_KEY[0]:
			*base = 10;
			if (DEC_KEY != "") *arg = strtok(*arg, DEC_KEY);
			break;

		// By default, handle any empty keys
		default:
			if (BIN_KEY == "") *base = 2;
			if (HEX_KEY == "") *base = 16;
			if (DEC_KEY == "") *base = 10;
			break;
	}
}

bool Parser::GetLabel(char* c)
{
	// Loop over all labels in label dictionary
	for (int i = 0; i < MAX_LABELS; i++)
	{
		// Convert label i in the label dictionary to const char*
		const char* lbl = _labels[i].c_str();

		// If the passed-in character matches lbl, it has been successfully found in the label dictionary
		if (!strcmp(c, lbl))
		{
			// Pull out the label from entry i in the label dictionary for use outside of this function
			_label = _labels[i];

			// Pull out the replacement value from entry i in the label dictionary for use outside of this function
			_replacementValue = _replacementValues[i];

			// Since we found a match, return true
			return true;
		}
	}

	// If we've looped through the whole label dictionary and didn't find a match, return false
	return false;
}

bool Parser::IsNumeric(char* c)
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

bool Parser::IsAMnemonic(char* c)
{
	string mnemonics[5] = { "nop", "mov", "lda", "tba", "stc" };

	// Just compare to mnemonics and return if there's a match
	bool mnemonic = false;
	for (int i = 0; i < 5; i++)
		mnemonic = mnemonic || !strcmp(c, mnemonics[i].c_str());

	return mnemonic;
}