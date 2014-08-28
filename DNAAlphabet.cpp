#include "DNAAlphabet.h"

DNAAlphabet::DNAAlphabet(): Alphabet()
{	
	Alphabet::unifiedAlphabet[65]='A';
	Alphabet::unifiedAlphabet[97]='A';

	Alphabet::unifiedAlphabet[67]='C';
	Alphabet::unifiedAlphabet[99]='C';

	Alphabet::unifiedAlphabet[71]='G';
	Alphabet::unifiedAlphabet[103]='G';

	Alphabet::unifiedAlphabet[84]='T';
	Alphabet::unifiedAlphabet[116]='T';

	Alphabet::alphabetToBinID[(int)'A']=1;
	Alphabet::alphabetToBinID[(int)'C']=2;
	Alphabet::alphabetToBinID[(int)'G']=3;
	Alphabet::alphabetToBinID[(int)'T']=4;
}

char DNAAlphabet::getSentinel()
{
	for(int i=0;i<256;i++)
	{
		if(Alphabet::unifiedAlphabet[i] >0)
		{
			Alphabet::alphabetToBinID[i-1]=0;  //bin 0 for suffixes starting with sentinel
			return ((char)(i-1));
		}
	}
	return '0';
}

void DNAAlphabet::setSentinel()
{
	for(int i=0;i<256;i++)
	{
		if(Alphabet::unifiedAlphabet[i] >0)
		{
			Alphabet::alphabetToBinID[i-1]=0;  //bin 0 for suffixes starting with sentinel			
		}
	}	
}
short DNAAlphabet::getSize()
{
	return 4;
}

char  DNAAlphabet::getUnifiedCharacter(char originalCharacter)
{
	return Alphabet::unifiedAlphabet[(short)originalCharacter];
}

short DNAAlphabet::getBinID (char alphabetChar)
{
	return Alphabet::alphabetToBinID[(short)alphabetChar];
}
