#include <vector>
#include <string>

using namespace std;

class ROMData
{
public:
	ROMData();

	void AddEntry(int address, int value);
	void AddEntryToCurrentAddress(int value);
	void SetArchitecture(const string& arch);
	void SetStartAddress(int a);
	void SetCurrentAddress(int a);
	void IncrementCurrentAddress(int n) { _currAddress += n; }
	void SetEndAddress(int a);
	int GetCurrentAddress() { return _currAddress; }
	bool GetValueAtAddress(int a, int *v);
	void SetPattern(const string& p);
	void PrintList();
	void PrintTable();
	void WriteProgram(const char* filename, const unsigned int size);
	void WriteControlROM();
	void SetBitWidth(int bw);
	void SetROMsize(int s);
	void SetROMname(string n);

private:
	int _bitWidth;
	int _romSize;
	int _currAddress;
	int _startAddress;
	int _endAddress;
	vector<int> _addresses;
	vector<int> _values;
	vector<string> _patterns;
	string _architecture;
	string _romName;
};
