#include "OpcodeDictionary.h"

OpcodeDictionary::OpcodeDictionary()
{
	currMnemonic = "";
	currArg0type = ArgType::None;
	currArg1type = ArgType::None;
	currArg0string = "";
	currArg1string = "";
	currValue = -1;
	currNumArgs = -1;
	currSize = 0;

	_mnemonics.clear();
	_numArgs.clear();
	_values.clear();
	_arg0types.clear();
	_arg1types.clear();
	_arg0strings.clear();
	_arg1strings.clear();
	_sizes.clear();
}

int OpcodeDictionary::NumOpcodes()
{
	return _mnemonics.size();
}

void OpcodeDictionary::AddCurrentEntry()
{
	_mnemonics.push_back(currMnemonic);
	_numArgs.push_back(currNumArgs);
	_values.push_back(currValue);
	_arg0types.push_back(currArg0type);
	_arg1types.push_back(currArg1type);
	_arg0strings.push_back(currArg0string);
	_arg1strings.push_back(currArg1string);
	_sizes.push_back(currSize);
}

void OpcodeDictionary::Add2Arg(string m, string a0, string a1, int s, int v)
{
	_mnemonics.push_back(m);
	_numArgs.push_back(2);
	_arg0types.push_back(ArgType::Register);
	_arg1types.push_back(ArgType::Register);
	_arg0strings.push_back(a0);
	_arg1strings.push_back(a1);
	_values.push_back(v);
	_sizes.push_back(s);

	currMnemonic = m;
	currNumArgs = 2;
	currArg0type = ArgType::Register;
	currArg1type = ArgType::Register;
	currArg0string = a0;
	currArg1string = a1;
	currValue = v;
	currSize = s;
}

void OpcodeDictionary::Add2Arg(string m, string a0, int a1, int s, int v)
{
	_mnemonics.push_back(m);
	_numArgs.push_back(2);
	_arg0types.push_back(ArgType::Register);
	_arg1types.push_back(ArgType::Numeral);
	_arg0strings.push_back(a0);
	_arg1strings.push_back("");
	_values.push_back(v);
	_sizes.push_back(s);

	currMnemonic = m;
	currNumArgs = 2;
	currArg0type = ArgType::Register;
	currArg1type = ArgType::Numeral;
	currArg0string = a0;
	currArg1string = "";
	currValue = v;
	currSize = s;
}

void OpcodeDictionary::Add2Arg(string m, int a0, string a1, int s, int v)
{
	_mnemonics.push_back(m);
	_numArgs.push_back(2);
	_arg0types.push_back(ArgType::Numeral);
	_arg1types.push_back(ArgType::Register);
	_arg0strings.push_back("");
	_arg1strings.push_back(a1);
	_values.push_back(v);
	_sizes.push_back(s);

	currMnemonic = m;
	currNumArgs = 2;
	currArg0type = ArgType::Numeral;
	currArg1type = ArgType::Register;
	currArg0string = "";
	currArg1string = a1;
	currValue = v;
	currSize = s;
}

void OpcodeDictionary::Add1Arg(string m, string a0, int s, int v)
{
	_mnemonics.push_back(m);
	_numArgs.push_back(1);
	_arg0types.push_back(ArgType::Register);
	_arg1types.push_back(ArgType::None);
	_arg0strings.push_back(a0);
	_arg1strings.push_back("");
	_values.push_back(v);
	_sizes.push_back(s);

	currMnemonic = m;
	currNumArgs = 1;
	currArg0type = ArgType::Register;
	currArg1type = ArgType::None;
	currArg0string = a0;
	currArg1string = "";
	currValue = v;
	currSize = s;
}

void OpcodeDictionary::Add1Arg(string m, int a0, int s, int v)
{
	_mnemonics.push_back(m);
	_numArgs.push_back(1);
	_arg0types.push_back(ArgType::Numeral);
	_arg1types.push_back(ArgType::None);
	_arg0strings.push_back("");
	_arg1strings.push_back("");
	_values.push_back(v);
	_sizes.push_back(s);

	currMnemonic = m;
	currNumArgs = 1;
	currArg0type = ArgType::Numeral;
	currArg1type = ArgType::None;
	currArg0string = "";
	currArg1string = "";
	currValue = v;
	currSize = s;
}

void OpcodeDictionary::Add0Arg(string m, int s, int v)
{
	_mnemonics.push_back(m);
	_numArgs.push_back(0);
	_arg0types.push_back(ArgType::None);
	_arg1types.push_back(ArgType::None);
	_arg0strings.push_back("");
	_arg1strings.push_back("");
	_values.push_back(v);
	_sizes.push_back(s);

	currMnemonic = m;
	currNumArgs = 1;
	currArg0type = ArgType::None;
	currArg1type = ArgType::None;
	currArg0string = "";
	currArg1string = "";
	currValue = v;
	currSize = s;
}

bool OpcodeDictionary::Get0ArgOpcode(string m, int *s, int* v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		string mnemonic = _mnemonics[i];

		if (m == mnemonic)
		{
			if (_numArgs[i] == 0)
			{
				*s = _sizes[i];
				*v = _values[i];

				return true;
			}
		}
	}

	return false;
}

bool OpcodeDictionary::Get1ArgOpcode(string m, string a0, int *s, int* v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		string mnemonic = _mnemonics[i];

		if (m == mnemonic)
		{
			if (_numArgs[i] == 1 && _arg0types[i] == ArgType::Register && _arg0strings[i] == a0)
			{
				*s = _sizes[i];
				*v = _values[i];

				return true;
			}
		}
	}

	return false;
}

bool OpcodeDictionary::Get1ArgOpcode(string m, int a0, int *s, int* v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		string mnemonic = _mnemonics[i];

		if (m == mnemonic)
		{
			if (_numArgs[i] == 1 && _arg0types[i] == ArgType::Numeral)
			{
				*s = _sizes[i];
				*v = _values[i];

				return true;
			}
		}
	}

	return false;
}

bool OpcodeDictionary::Get2ArgOpcode(string m, string a0, string a1, int *s, int* v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		string mnemonic = _mnemonics[i];

		if (m == mnemonic)
		{
			if (_numArgs[i] == 2 && _arg0types[i] == ArgType::Register && _arg0strings[i] == a0 &&
				_arg1types[i] == ArgType::Register && _arg1strings[i] == a1)
			{
				*s = _sizes[i];
				*v = _values[i];

				return true;
			}
		}
	}

	return false;
}

bool OpcodeDictionary::Get2ArgOpcode(string m, string a0, int a1, int *s, int* v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		string mnemonic = _mnemonics[i];

		if (m == mnemonic)
		{
			if (_numArgs[i] == 2 && _arg0types[i] == ArgType::Register && _arg0strings[i] == a0 &&
				_arg1types[i] == ArgType::Numeral)
			{
				*s = _sizes[i];
				*v = _values[i];

				return true;
			}
		}
	}

	return false;
}

bool OpcodeDictionary::Get2ArgOpcode(string m, int a0, string a1, int *s, int* v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		string mnemonic = _mnemonics[i];

		if (m == mnemonic)
		{
			if (_numArgs[i] == 2 && _arg0types[i] == ArgType::Numeral &&
				_arg1types[i] == ArgType::Register && _arg1strings[i] == a1)
			{
				*s = _sizes[i];
				*v = _values[i];

				return true;
			}
		}
	}

	return false;
}

bool OpcodeDictionary::GetOpcodeValue(int v)
{
	for (int i = 0; i < _mnemonics.size(); i++)
	{
		if (_values[i] == v)
		{
			return true;
		}
	}

	return false;
}

bool OpcodeDictionary::IsAMnemonic(char* c)
{
	bool mnemonic = false;
	for (int i = 0; i < _mnemonics.size(); i++)
		mnemonic = mnemonic || !strcmp(c, _mnemonics[i].c_str());

	return mnemonic;
}