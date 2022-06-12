#pragma once
#include <vector>
#include <string>
#include "Config.h"
#include "LabelDictionary.h"
#include "OpcodeDictionary.h"
#include "ROMData.h"

using namespace std;

enum class ParseMode { None, Architecture, Assembler };
enum class LineType { None, Blank, Comment, File, Register, Opcode, Directive, Symbol, Label, OpCode };
enum class TokenType { None, Architecture, Origin, Export, Byte, Ascii, Symbol, Label, OpCode };
enum class OutMode { None, Brief, Verbose };

class Parser
{
public:
	Parser();

	void SetParseMode(ParseMode m) { _parseMode = m; }
	void ResetParser() { _archProcessed = false; _archFile = ""; _numTokens = 0; _tokens.clear(); _lineType = LineType::None; _outMode = OutMode::None; _currTokenType = TokenType::None; };
	void ResetForNewLine() { _numTokens = 0; _tokens.clear(); _lineType = LineType::None; }
	void Parse(char* filename);
	void SetOutMode(OutMode m) { _outMode = m; }

protected:
	void ParseLineIntoTokens(const char* line, const char* delimiters);
	int ParseToken(int i);
	void CalculateBase(int i, int* base, char** arg);
	bool IsNumeric(char* c);
	vector<string> SplitString(string s, string delimiter);

private:
	bool _archProcessed = false;
	string _archFile;
	OutMode _outMode;
	ParseMode _parseMode;
	LineType _lineType;
	int     _numTokens;
	TokenType _currTokenType;
	vector<char*>   _tokens;
	LabelDictionary _labelDictionary;
	LabelDictionary _registerDictionary;
	OpcodeDictionary _opcodeDictionary;
	ROMData _programROM;
};
