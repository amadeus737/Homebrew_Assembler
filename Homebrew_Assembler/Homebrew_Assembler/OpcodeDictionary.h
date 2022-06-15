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
	void Add2Arg(const string& m, const string& a0, const string& a1, int s, int v);
	void Add2Arg(const string& m, const string& a0, int a1, int s, int v);
	void Add2Arg(const string& m, int a0, const string& a1, int s, int v);
	void Add1Arg(const string& m, const string& a0, int s, int v);
	void Add1Arg(const string& m, int a0, int s, int v);
	void Add0Arg(const string& m, int s, int v);
	bool Get0ArgOpcode(const string& m, int *s, int* v);
	bool Get1ArgOpcode(const string& m, const string& a0, int *s, int* v);
	bool Get1ArgOpcode(const string& m, int a0, int *s, int* v);
	bool Get2ArgOpcode(const string& m, const string& a0, const string& a1, int *s, int* v);
	bool Get2ArgOpcode(const string& m, const string& a0, int a1, int *s, int* v);
	bool Get2ArgOpcode(const string& m, int a0, const string& a1, int *s, int* v);
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
	int currSize;

private:
	vector<string> _mnemonics;
	vector<int> _values;
	vector<int> _numArgs;
	vector<int> _sizes;
	vector<ArgType> _arg0types;
	vector<ArgType> _arg1types;
	vector<string> _arg0strings;
	vector<string> _arg1strings;
};