#pragma once
#include <vector>
#include "Config.h"

using namespace std;

// Used to set the output print mode
enum class Mode { None, Brief, Verbose };

// Parsing in this assembler occurs in three phases. These enumerations are used to keep
// track of the types in each step. The three phases of parsing are:
//  (1) Parsing a file into individual lines
//  (2) Parsing each line into tokens
//  (3) Parsing tokens 
enum class LineType  { None, Blank, Comment, Directive, Symbol, Label, OpCode };
enum class TokenType { None, Origin, Export, Symbol, Label, OpCode };

class Parser
{
public:
	// Constructor
	Parser() { _numTokens = 0; _tokens.clear(); _lineType = LineType::None; _mode = Mode::None; _address = 0; _startAddress = 0; _endAddress = 0; _ocmnemonic = NULL; _currTokenType = TokenType::None; _label = ""; _replacementValue = -1; _labelCount = -1; for (int i = 0; i < 10; i++) { _labels[i] = ""; _replacementValues[i] = -1; _newOpCode = false; } }

	// This function initiates phase 0 of parsing (file -> lines)
	void Parse(char* filename);

	// This function sets the output print mode
	void SetMode(Mode m) { _mode = m; }

private:
	// This function initiates phase 1 of parsing (lines -> tokens)
	void ParseLineIntoTokens(const char* line, const char* delimiters);

	// This function initiates phase 2 of parsing (tokens -> interpreted assembly)
	void ParseToken(int i);

	// Resets some parameters
	void Reset() { _numTokens = 0; _tokens.clear(); _lineType = LineType::None; }

	// Used to calculate the numerical base 
	void CalculateBase(int i, int* base, char** arg);

	// Used to compare an input character with label database and retrieve both label and replacement value
	bool GetLabel(char* c);

	// Tests an input character for 0-9 and all number prefixes (BIN_KEY, HEX_KEY, DEC_KEY)
	bool IsNumeric(char* c);

	// Tests whether the input character is an opcode mnemonic
	bool IsAMnemonic(char* c);

private:
	Mode _mode;							 // Holds the output print mode
	int _address;						 // Holds the calculated address, which can be modified by some assembly commands
	int _startAddress;                   // The start address for writing to the ROM
	int _endAddress;                     // The end address for writing to the ROM
	LineType _lineType;                  // Type of line being parsed
	int     _numTokens;                  // Number of tokes in a line
	vector<char*>   _tokens;             // Holds the parsed tokens for the line
	TokenType _currTokenType;            // Holds the type of the current token
	char* _ocmnemonic;                   // 
	int _labelCount;                     // Used to index entries in the label dictionary (_labels and _replacementValues)
	string _labels[MAX_LABELS];          // Holds the labels added to the label dictionary
	int _replacementValues[MAX_LABELS];  // Holds the values string labels in the label dictionary are replaced with
	string _label;                       //
	int _replacementValue;               //
	bool _newOpCode;                     //
};