#include <string>
#include <vector>

using namespace std;

enum class ArgType { None, Register, Numeral };

class OpcodeDictionary
{
public:
	OpcodeDictionary();

	int NumOpcodes();
	void AddCurrentEntry();
	void Add2Arg(string m, string a0, string a1, int v);
	void Add2Arg(string m, string a0, int a1, int v);
	void Add2Arg(string m, int a0, string a1, int v);
	void Add1Arg(string m, string a0, int v);
	void Add1Arg(string m, int a0, int v);
	void Add0Arg(string m, int v);
	bool Get0ArgOpcode(string m, int* v);
	bool Get1ArgOpcode(string m, string a0, int* v);
	bool Get1ArgOpcode(string m, int a0, int* v);
	bool Get2ArgOpcode(string m, string a0, string a1, int* v);
	bool Get2ArgOpcode(string m, string a0, int a1, int* v);
	bool Get2ArgOpcode(string m, int a0, string a1, int* v);
	bool GetOpcodeValue(int v);
	bool IsAMnemonic(char* c);

	string currMnemonic;
	int currValue;
	ArgType currArg0type;
	ArgType currArg1type;
	string currArg0string;
	string currArg1string;
	int currNumArgs;
	int currArg0num;
	int currArg1num;

private:
	vector<string> _mnemonics;
	vector<int> _values;
	vector<int> _numArgs;
	vector<ArgType> _arg0types;
	vector<ArgType> _arg1types;
	vector<string> _arg0strings;
	vector<string> _arg1strings;
};