#pragma once
#include <vector>
#include <cstring>
#include <string>

#define MAX_LABELS 10

using namespace std;

enum class LineType { None, Blank, Comment, Directive, Symbol, Label, OpCode };
enum class TokenType { None, Origin, Export, Symbol, Label, OpCode };
enum class Mode { None, Brief, Verbose };

class Parser
{
public:
	Parser() { _numTokens = 0; _tokens.clear(); _tokenTypes.clear(); _lineType = LineType::None; _mode = Mode::None; _address = 0; _startAddress = 0; _endAddress = 0; _ocmnemonic = NULL; _currTokenType = TokenType::None; _lastTokenType = TokenType::None; _label = ""; _replacementValue = -1; _labelCount = -1; for (int i = 0; i < 10; i++) { _labels[i] = ""; _replacementValues[i] = -1; _newOpCode = false; } }

	void Parse(char* filename);
	void SetMode(Mode m) { _mode = m; }

//	LineType GetLineType() { return _lineType; }
//	const char* GetLineTypeString() { return Types[(int)_lineType]; }
//	int NumTokens() { return _numTokens; }
//	vector<char*> GetTokens() { return _tokens; }
//	char* GetToken(int i) { return _tokens[i]; }
//	LineType GetTokenType(int i) { return _tokenTypes[i]; }

private:
	void ParseLineIntoTokens(const char* line, const char* delimiters);
	void ParseToken(int i);
	void Reset() { _numTokens = 0; _tokens.clear(); _tokenTypes.clear(); _lineType = LineType::None; }
	void CalculateBase(int i, int* base, char** arg);
	bool GetLabel(char* c);
	bool IsNumeric(char* c);
	bool IsAMnemonic(char* c);

private:
	LineType _lineType;
	int     _numTokens;
	vector<char*>   _tokens;
	vector<LineType> _tokenTypes;
	const char* Types[7] = { "None", "Blank", "Comment", "Directive", "Symbol", "Label", "OpCode" };
	Mode _mode;
	int _address;
	int _startAddress;
	int _endAddress;
	char* _ocmnemonic;
	TokenType _currTokenType;
	TokenType _lastTokenType;
	int _labelCount;
	string _labels[10];
	int _replacementValues[10];
	string _label;
	int _replacementValue;
	bool _newOpCode;
};
