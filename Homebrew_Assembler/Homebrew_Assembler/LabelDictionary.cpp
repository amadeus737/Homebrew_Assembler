#include "LabelDictionary.h"

LabelDictionary::LabelDictionary()
{
	currLabel = "";
	currValue = 0;

	_labels.clear();
	_values.clear();
}

int LabelDictionary::NumLabels()
{
	return _labels.size();
}

void LabelDictionary::AddCurrentEntry()
{
	_labels.push_back(currLabel);
	_values.push_back(currValue);
}

void LabelDictionary::Add(const string& newLabel, int newVal)
{
	_labels.push_back(newLabel);
	_values.push_back(newVal);

	currLabel = newLabel;
	currValue = newVal;
}

bool LabelDictionary::GetLabel(const char* c)
{
	for (int i = 0; i < _labels.size(); i++)
	{
		const char* lbl = _labels[i].c_str();

		if (!strcmp(c, lbl))
		{
			currLabel = lbl;
			currValue = _values[i];

			return true;
		}
	}

	return false;
}