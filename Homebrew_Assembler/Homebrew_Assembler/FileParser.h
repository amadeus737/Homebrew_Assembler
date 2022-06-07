#pragma once
#include <vector>
#include <cstring>

using namespace std;

enum class LineType { None, Blank, Comment, Directive, Symbol, Label, OpCode };

class FileParser
{
public:
	FileParser() { _numTokens = 0; _tokens.clear(); _tokenTypes.clear(); _lineType = LineType::None; }

	void Parse(const char* line, const char* delimiters);
	void Reset() { _numTokens = 0; _tokens.clear(); _tokenTypes.clear(); _lineType = LineType::None; }

	LineType GetLineType() { return _lineType; }
	const char* GetLineTypeString() { return Types[(int)_lineType]; }
	int NumTokens() { return _numTokens; }
	vector<char*> GetTokens() { return _tokens; }
	char* GetToken(int i) { return _tokens[i]; }
	LineType GetTokenType(int i) { return _tokenTypes[i]; }

private:
	LineType _lineType;
	int     _numTokens;
	vector<char*>   _tokens;
	vector<LineType> _tokenTypes;
	const char* Types[7] = { "None", "Blank", "Comment", "Directive", "Symbol", "Label", "OpCode" };
};
