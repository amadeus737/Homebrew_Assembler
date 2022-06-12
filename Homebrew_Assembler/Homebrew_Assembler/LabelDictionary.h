#include <string>
#include <vector>

using namespace std;

class LabelDictionary
{
public:
	LabelDictionary();
	
	int NumLabels();
	void AddCurrentEntry();
	void Add(string newLabel, int newVal);
	bool GetLabel(char* c);

	string currLabel;
	int currValue;

private:
	vector<string> _labels;
	vector<int> _values;
};