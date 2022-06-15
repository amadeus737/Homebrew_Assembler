#include <vector>

using namespace std;

class ROMData
{
public:
	ROMData();

	void AddEntry(int address, int value);
	void AddEntryToCurrentAddress(int value);
	void SetStartAddress(int a);
	void SetCurrentAddress(int a);
	void IncrementCurrentAddress(int n) { _currAddress += n; }
	void SetEndAddress(int a);
	int GetCurrentAddress() { return _currAddress; }
	bool GetValueAtAddress(int a, int *v);
	void SetPattern(const string& p);
	void PrintList();
	void PrintTable();
	void Write(const char* filename, const unsigned int size);

private:
	int _currAddress;
	int _startAddress;
	int _endAddress;
	vector<int> _addresses;
	vector<int> _values;
	vector<string> _patterns;
};
