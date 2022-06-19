#pragma once
#include <vector>
#include <string>
#include "Config.h"
#include "LabelDictionary.h"
#include "OpcodeDictionary.h"
#include "ROMData.h"

using namespace std;

enum class ParseMode { None, Architecture, Assembler };
enum class LineType { None, Blank, Comment, File, ArchRegister, ArchOpcode, ArchControl, ArchControlAlias, Directive, Symbol, Label, OpCode };
enum class TokenType { None, Architecture, Include, Origin, Export, Byte, Ascii, Symbol, Label, OpCode };
enum class OutMode { None, Brief, Verbose };
enum class BinaryOperation { None, LogicalOR, LogicalAND };
class Parser
{
public:
	Parser() :
		_processingExternFile(false), _linePtr(-1), _currFile(""), _outMode(OutMode::None), _parseMode(ParseMode::None), _lineType(LineType::None), _numTokens(0),
		_currTokenType(TokenType::None), _labelDictionary(LabelDictionary()), _registerDictionary(LabelDictionary()), _opcodeDictionary(OpcodeDictionary()), _controlDictionary(LabelDictionary()), _programROM(ROMData()), _equalProcessed(false), _lastOperation(BinaryOperation::None), _ocValProcessed(false), _opcodeIsAliased(false)
	{ _tokens.clear(); }

	void SetParseMode(ParseMode m) { _parseMode = m; }
	void ResetParser() { _linePtr = -1; _processingExternFile = false; _currFile = ""; _numTokens = 0; _tokens.clear(); _lineType = LineType::None; _outMode = OutMode::None; _currTokenType = TokenType::None; };
	void ResetForNewLine() { _numTokens = 0; _tokens.clear(); }
	void Parse(const char* filename);
	void SetOutMode(OutMode m) { _outMode = m; }

protected:
	void ParseLineIntoTokens(const char* line, const char* delimiters);
	int ParseToken(int i);
	void CalculateBase(int i, int* base, char** arg);
	bool IsNumeric(const char* c);
	const string SplitFilename(const string& s, const string& preferredPath, const string& preferredExtension, bool forcePreferred);
	void WriteToROM(const char* filename);

private:
	bool _processingExternFile;
	int _linePtr = -1;
	string _currFile;
	OutMode _outMode;
	ParseMode _parseMode;
	LineType _lineType;
	int     _numTokens;
	TokenType _currTokenType;
	vector<char*>   _tokens;
	LabelDictionary _labelDictionary;
	LabelDictionary _registerDictionary;
	LabelDictionary _controlDictionary;
	OpcodeDictionary _opcodeDictionary;
	ROMData _programROM;
	BinaryOperation _lastOperation;
	bool _equalProcessed = false;
	bool _ocValProcessed = false;
	bool _opcodeIsAliased = false;
};
