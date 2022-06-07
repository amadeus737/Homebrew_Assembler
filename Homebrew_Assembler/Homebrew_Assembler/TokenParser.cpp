#include "TokenParser.h"
#include "Config.h"
#include <string>

void TokenParser::Parse(const char* line, const char* delimiters)
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
