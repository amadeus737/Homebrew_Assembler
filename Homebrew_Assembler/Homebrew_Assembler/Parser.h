#pragma once
#include <vector>
#include <cstring>
#include <string>
#include "Config.h"

//#define MAX_LABELS    100
//#define MAX_REGISTERS 50
//#define MAX_OPCODES   65536  // 2^16 bc 16-bit datapath
//#define MAX_OPCODE_ARGS 2

using namespace std;

enum class ParseMode { None, Architecture, Assembler };
enum class LineType { None, Blank, Comment, File, Register, Opcode, Directive, Symbol, Label, OpCode };
enum class TokenType { None, Architecture, Origin, Export, Byte, Ascii, Symbol, Label, OpCode };
enum class OutMode { None, Brief, Verbose };
enum class ArgType { None, Register, Numeral };

class LabelDictionary
{
	public:
		LabelDictionary()
		{
			currLabel = "";
			currValue = 0;

			labels.clear();
			values.clear();
		}

		int NumLabels()
		{
			return labels.size();
		}

		void AddCurrentEntry() 
		{
			labels.push_back(currLabel);
			values.push_back(currValue);
		}

		void Add(string newLabel, int newVal) 
		{
			labels.push_back(newLabel);
			values.push_back(newVal);

			currLabel = newLabel;
			currValue = newVal;
		}

		string currLabel;
		int currValue;

		vector<string> labels;
		vector<int> values;
};

class OpcodeDictionary
{
	public:
		OpcodeDictionary()
		{
			currMnemonic = "";
			currArg0type = ArgType::None;
			currArg1type = ArgType::None;
			currArg0string = "";
			currArg1string = "";
			currValue = -1;
			currNumArgs = -1;

			mnemonics.clear();
			numArgs.clear();
			values.clear();
			arg0types.clear();
			arg1types.clear();
			arg0strings.clear();
			arg1strings.clear();
		}

		int NumOpcodes()
		{
			return mnemonics.size();
		}

		void AddCurrentEntry()
		{
			mnemonics.push_back(currMnemonic);
			numArgs.push_back(currNumArgs);
			values.push_back(currValue);
			arg0types.push_back(currArg0type);
			arg1types.push_back(currArg1type);
			arg0strings.push_back(currArg0string);
			arg1strings.push_back(currArg1string);
		}

		void Add2Arg(string m, string a0, string a1, int v)
		{
			mnemonics.push_back(m);
			numArgs.push_back(2);
			arg0types.push_back(ArgType::Register);
			arg1types.push_back(ArgType::Register);
			arg0strings.push_back(a0);
			arg1strings.push_back(a1);
			values.push_back(v);

			currMnemonic = m;
			currNumArgs = 2;
			currArg0type = ArgType::Register;
			currArg1type = ArgType::Register;
			currArg0string = a0;
			currArg1string = a1;
			currValue = v;
		}

		void Add2Arg(string m, string a0, int a1, int v)
		{
			mnemonics.push_back(m);
			numArgs.push_back(2);
			arg0types.push_back(ArgType::Register);
			arg1types.push_back(ArgType::Numeral);
			arg0strings.push_back(a0);
			arg1strings.push_back("");
			values.push_back(v);

			currMnemonic = m;
			currNumArgs = 2;
			currArg0type = ArgType::Register;
			currArg1type = ArgType::Numeral;
			currArg0string = a0;
			currArg1string = "";
			currValue = v;
		}

		void Add2Arg(string m, int a0, string a1, int v)
		{
			mnemonics.push_back(m);
			numArgs.push_back(2);
			arg0types.push_back(ArgType::Numeral);
			arg1types.push_back(ArgType::Register);
			arg0strings.push_back("");
			arg1strings.push_back(a1);
			values.push_back(v);

			currMnemonic = m;
			currNumArgs = 2;
			currArg0type = ArgType::Numeral;
			currArg1type = ArgType::Register;
			currArg0string = "";
			currArg1string = a1;
			currValue = v;
		}

		void Add1Arg(string m, string a0, int v)
		{
			mnemonics.push_back(m);
			numArgs.push_back(1);
			arg0types.push_back(ArgType::Register);
			arg1types.push_back(ArgType::None);
			arg0strings.push_back(a0);
			arg1strings.push_back("");
			values.push_back(v);

			currMnemonic = m;
			currNumArgs = 1;
			currArg0type = ArgType::Register;
			currArg1type = ArgType::None;
			currArg0string = a0;
			currArg1string = "";
			currValue = v;
		}
		
		void Add1Arg(string m, int a0, int v)
		{
			mnemonics.push_back(m);
			numArgs.push_back(1);
			arg0types.push_back(ArgType::Numeral);
			arg1types.push_back(ArgType::None);
			arg0strings.push_back("");
			arg1strings.push_back("");
			values.push_back(v);

			currMnemonic = m;
			currNumArgs = 1;
			currArg0type = ArgType::Numeral;
			currArg1type = ArgType::None;
			currArg0string = "";
			currArg1string = "";
			currValue = v;
		}

		void Add1Arg(string m, int v)
		{
			mnemonics.push_back(m);
			numArgs.push_back(0);
			arg0types.push_back(ArgType::None);
			arg1types.push_back(ArgType::None);
			arg0strings.push_back("");
			arg1strings.push_back("");
			values.push_back(v);

			currMnemonic = m;
			currNumArgs = 1;
			currArg0type = ArgType::None;
			currArg1type = ArgType::None;
			currArg0string = "";
			currArg1string = "";
			currValue = v;
		}

		string currMnemonic;
		int currValue;
		ArgType currArg0type;
		ArgType currArg1type;
		string currArg0string;
		string currArg1string;
		int currNumArgs;
		int currArg0num;
		int currArg1num;

		vector<string> mnemonics;
		vector<int> values;
		vector<int> numArgs;
		vector<ArgType> arg0types;
		vector<ArgType> arg1types;
		vector<string> arg0strings;
		vector<string> arg1strings;
};

class Parser
{
public:
	Parser();

	void SetParseMode(ParseMode m) { _parseMode = m; }
	void ResetParser() { _archProcessed = false; _archFile = ""; _numTokens = 0; _tokens.clear(); _lineType = LineType::None; _outMode = OutMode::None; _address = 0; _startAddress = 0; _endAddress = 0; _currTokenType = TokenType::None; };
	void ResetForNewLine() { _numTokens = 0; _tokens.clear(); _lineType = LineType::None; }
	void Parse(char* filename);
	void SetOutMode(OutMode m) { _outMode = m; }

protected:
	void ParseLineIntoTokens(const char* line, const char* delimiters);
	int ParseToken(int i);
	void CalculateBase(int i, int* base, char** arg);
	bool GetLabel(char* c);
	bool GetRegister(char* c);
	bool IsNumeric(char* c);
	bool IsAMnemonic(char* c);

private:
	bool _archProcessed = false;
	string _archFile;
	OutMode _outMode;
	ParseMode _parseMode;
	LineType _lineType;
	int     _numTokens;
	vector<char*>   _tokens;
	TokenType _currTokenType;
	int _address;
	int _startAddress;
	int _endAddress;
	//char* _ocmnemonic;
	bool _newOpCode;

	LabelDictionary _labelDictionary;
	LabelDictionary _registerDictionary;
	OpcodeDictionary _opcodeDictionary;
};
