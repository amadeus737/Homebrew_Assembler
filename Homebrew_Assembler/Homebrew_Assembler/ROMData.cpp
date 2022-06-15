#include "ROMData.h"

ROMData::ROMData()
{
	_currAddress = 0;
	_startAddress = 0;
	_endAddress = 0;
	_addresses.clear();
	_values.clear();
	_patterns.clear();
}

void ROMData::AddEntry(int address, int value)
{
	_addresses.push_back(address);
	_values.push_back(value);
}

void ROMData::AddEntryToCurrentAddress(int value)
{
	_addresses.push_back(_currAddress);
	_values.push_back(value);
}

void ROMData::SetStartAddress(int a)
{
	_startAddress = a;
}

void ROMData::SetCurrentAddress(int a)
{
	_currAddress = a;
}

void ROMData::SetEndAddress(int a)
{
	_endAddress = a;
}

void ROMData::SetPattern(const string& p)
{
	_patterns.push_back(p);
}

void ROMData::PrintTable()
{
	printf("\n----------------------------------------------------\n");
	printf("                  ROM DATA TABLE\n");
	printf("----------------------------------------------------");

	int columnCount = 0;
	int lastVal = -1;
	int currRow = 0;
	int lastRow = -1;
	int ellipsisPrinted = false;
	for (int i = _startAddress; i <= _endAddress; i++)
	{
		int addressCalc = _startAddress + i * 8;

		if (columnCount == 0 || columnCount == 16)
		{
			printf("\n%02x:", i);
			columnCount = 0;

			currRow++;
		}

		int v = -1;
		if (i < _addresses.size() && i < _values.size() && GetValueAtAddress(i, &v))
		{
			printf(" %02x", v);
			lastVal = v;			
		}
		else
		{
			if (lastRow == currRow && lastVal != 0)
				printf(" %02x", 0);
			else if (i == _endAddress && lastVal == 0)
				printf(" %02x", 0);			
			else if (!ellipsisPrinted)
			{
				printf(" ..");
			}

			lastVal = 0;
		}

		lastRow = currRow;
		columnCount++;
	}

	printf("\n\n\n");
}

void ROMData::PrintList() 
{
	printf("-------------------------\n");
	printf("     PROGRAM LISTING\n");
	printf("-------------------------\n");

	int lastVal = -1;
	int ellipsisPrinted = false;
	for (int i = _startAddress; i <= _endAddress; i++)
	{
		int addressCalc = i;

		int v = -1;
		if (i < _addresses.size() && i < _values.size() && GetValueAtAddress(addressCalc, &v))
		{
			printf("%02x: %02x   (%s)\n", addressCalc, v, _patterns[i].c_str());
			lastVal = v;
		}
		else
		{
			if (lastVal != 0)
				printf("%02x: %02x   (empty)\n", addressCalc, 0);
			else if (lastVal == 0 && i == _endAddress)
				printf("%02x: %02x   (empty)\n", addressCalc, 0);
			else if (!ellipsisPrinted)
			{
				printf("  ...\n");
				ellipsisPrinted = true;
			}				

			lastVal = 0;
		}
	}
	
	printf("\n");
}

bool ROMData::GetValueAtAddress(int a, int* v)
{
	for (int i = 0; i < _addresses.size(); i++)
	{
		if (a == _addresses[i])
		{
			*v = _values[i];
			return true;
		}
	}

	return false;
}

void ROMData::Write(const char* filename, const unsigned int size)
{
	// Create the binary file
	FILE *file = fopen(filename, "wb");
	if (!file)
	{
		printf("!!! CRITICAL ERROR: Cannot open file %s for writing to ROM !!!\n", filename);
		return;
	}

	// unsigned char bc its size is 1 byte = 8 bits
	unsigned char* romData = new unsigned char[size];  // 32768 = 32 KBytes = 256 KBits
	memset(romData, 0x00, size);  // Set 32768 bytes to 0x00

	// copy from _values to romData
	for (int i = 0; i < _values.size(); i++)
	{
		romData[i] = (unsigned char)_values[i];
	}

	// Write data to binary file (will be written to ROM via TL86II Plus Programmer)
	fwrite(romData, 1, 32768, file);
	fclose(file);

	delete [] romData;
}