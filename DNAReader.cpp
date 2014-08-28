#include "DNAReader.h"

using namespace std;

DNAReader::DNAReader() :InputReader()
{
	this->initAlphabet();
}
	

void DNAReader::initAlphabet()
{
	//sets up an evaluation table for input characters - in this case they may be 0-127
	for(int i=0; i<128;i++)
	{
		this->validAlphabet[i]=-1; //invalid	
	};

	//set dna characters A,a - 1, C,c -2, G,g -3, T,t -4, sentinel $ - 0
	this->validAlphabet[65]=1;
	this->validAlphabet[97]=1;

	this->validAlphabet[67]=2;
	this->validAlphabet[99]=2;

	this->validAlphabet[71]=3;
	this->validAlphabet[103]=3;

	this->validAlphabet[84]=4;
	this->validAlphabet[116]=4;

	this->validAlphabet[36]=0;

	//now special characters encoding end of line
	this->validAlphabet[10]=-10;
	this->validAlphabet[13]=-10;

	InputReader::alphabet = new DNAAlphabet();
}
	
bool DNAReader::validLine(const string&  line)
{
	int strLength = line.size();
	for(int i=0; i<strLength; i++)
	{
		short charAsInt = (short)line.at(i);
        if (line[i]>=128 || this->validAlphabet[charAsInt] <=0)
			return false;
	}
	return true;
}



