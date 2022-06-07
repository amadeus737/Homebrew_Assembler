#include "Parser.h"
#include "Config.h"
#include <string>
#include <fstream>

void Parser::Parse(char* filename)
{
	if (_mode == Mode::Verbose)
	{
		printf(" -> Parsing: \"%s\"\n", filename);
	}

	// Used to keep track of line that is currently being parsed
	int linenum = 0;
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
		_lastTokenType = TokenType::None;
		_currTokenType = TokenType::None;

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
					printf("\n    -> #%d \"%s\"\n", linenum, c_line);
				}

				// Parse line into tokens
				ParseLineIntoTokens(c_line, " ,\t");

				if (_numTokens > 0)
				{
					char* label = NULL;
					TokenType tokenType = TokenType::None;
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
	// This command actually only performs one split...hence the while loop below
	char* tokens = strtok((char*)line, delimiters);
	_tokens.push_back(tokens);

	if (tokens == NULL)
	{
		_lineType = LineType::Blank;
		_tokenTypes.push_back(_lineType);
	}
	else if (tokens != NULL && tokens[0] == ';')
	{
		_lineType = LineType::Comment;
		_tokenTypes.push_back(_lineType);
	}
	else
	{
		switch (tokens[0])
		{
		case DIRECTIVE_KEYS[0]:
			_lineType = LineType::Directive;
			_tokenTypes.push_back(_lineType);
			break;

		case SYMBOL_KEYS[0]:
			_lineType = LineType::Symbol;
			_tokenTypes.push_back(_lineType);
			break;

		case LABEL_KEYS[0]:
			_lineType = LineType::Label;
			_tokenTypes.push_back(_lineType);
			break;

		default:
			_lineType = LineType::OpCode;
			_tokenTypes.push_back(_lineType);
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

void Parser::ParseToken(int i)
{
	if (i >= _numTokens) return;

	char* directive_parse = strtok(_tokens[i], DIRECTIVE_KEYS);
	char* symbol_parse = strtok(_tokens[i], SYMBOL_KEYS);
	char* label_parse = strtok(_tokens[i], LABEL_KEYS);

	if (!strcmp(directive_parse, ORIGIN_STR))
	{
		_currTokenType = TokenType::Origin;
	}
	else if (!strcmp(directive_parse, EXPORT_STR))
	{
		_currTokenType = TokenType::Export;
	}
	else if (strcmp(symbol_parse, _tokens[i]))
	{
		_currTokenType = TokenType::Symbol;

		_labelCount++;
		_label = symbol_parse;
		_labels[_labelCount] = _label;
	}
	else if (strcmp(label_parse, _tokens[i]))
	{
		_currTokenType = TokenType::Label;

		_labelCount++;
		_label = label_parse;
		_labels[_labelCount] = _label;
	}

	// Only other token type to process is an OpCode!
	if (_lineType == LineType::OpCode)
	{
		if (IsAMnemonic(_tokens[i]))
		{		
			// Later, replace with actual size of opcode
			_address += 8;
			_currTokenType = TokenType::OpCode;

			_ocmnemonic = _tokens[i];
			_newOpCode = true;
			printf("      -- ocmnemoic is %s\n", _ocmnemonic);
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
			printf("      -- Address set to: %x\n", _address);

			_lastTokenType = _currTokenType;
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
			printf("      -- Start Address set to: %x\n", _startAddress);
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
		}

			CalculateBase(i + 1, &base, &arg);

		if (base != -1)
		{
			_endAddress = stoi(arg, nullptr, base);
			printf("      -- End Address set to: %x\n", _endAddress);

			_lastTokenType = _currTokenType;
			_currTokenType = TokenType::None;
		}
		else
		{
			printf("      -- ERROR occurred in parsing argument for EXPORT directive\n");
		}
	}

	if (_currTokenType == TokenType::Symbol && !strcmp(symbol_parse, _tokens[i]))
	{
		if (base != -1)
		{
			int replacementValue = stoi(arg, nullptr, base);
			printf("      -- Symbol: %s = %x\n", _label.c_str(), replacementValue);
			_replacementValues[_labelCount] = replacementValue;

			_lastTokenType = _currTokenType;
			_currTokenType = TokenType::None;
		}
	}

	if (_currTokenType == TokenType::Label)
	{
		if (base != -1)
		{
			int replacementValue = _address;
			printf("      -- Label: %s = %x\n", _label.c_str(), replacementValue);

			_replacementValues[_labelCount] = replacementValue;

			_lastTokenType = _currTokenType;
			_currTokenType = TokenType::None;
		}
	}
	
	if (_currTokenType == TokenType::OpCode)
	{
		if (strcmp(arg, _ocmnemonic) && _newOpCode)
		{
			_newOpCode = false;

			if (IsNumeric(arg))
			{
				if (base != -1)
				{
					int arg0 = stoi(arg, nullptr, base);
					printf("      -- arg0 = %x\n", arg0);

					_address += 8;
				}
			}
			else
			{
				if (!GetLabel(arg))
					printf("      -- arg0 is a REGISTER %s\n", arg);
				else
				{
					printf("      -- arg0 is a LABEL %x\n", _replacementValue);
					_address += 8;
				}
			}

			CalculateBase(i + 1, &base, &arg);

			if (IsNumeric(arg))
			{
				if (base != -1)
				{
					int arg1 = stoi(arg, nullptr, base);
					printf("      -- arg1 = %x\n", arg1);

					_address += 8;
				}
			}
			else
			{
				if (!GetLabel(arg))
				{
					printf("      -- arg1 is a REGISTER %s\n", arg);
					printf("      -- Address = %x\n", _address);

					_lastTokenType = _currTokenType;
					_currTokenType = TokenType::None;
				}
				else
				{
					_address += 8;
					printf("      -- arg1 is a LABEL %x\n", _replacementValue);
					printf("      -- Address = %x\n", _address);

					_lastTokenType = _currTokenType;
					_currTokenType = TokenType::None;
				}
			}
		}
	}
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

bool Parser::GetLabel(char* c)
{
	//printf("c is %s\n", c);
	for (int i = 0; i < 10; i++)
	{
		//printf("checking label -- %s\n", labels[i].c_str());
		const char* lbl = _labels[i].c_str();
		//printf("lbl %s\n", lbl);

		if (!strcmp(c, lbl))
		{
			_label = _labels[i];
			_replacementValue = _replacementValues[i];

			return true;
		}
	}

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
	string mnemonics[2] = { "nop", "mov" };

	bool mnemonic = false;
	for (int i = 0; i < 2; i++)
		mnemonic = mnemonic || !strcmp(c, mnemonics[i].c_str());

	return mnemonic;
}