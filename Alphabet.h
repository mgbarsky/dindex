#ifndef ALPHABET_H
#define ALPHABET_H

class Alphabet
{
public:
	Alphabet();
virtual ~Alphabet(){}
	virtual char getSentinel()=0;
	virtual short getSize()=0;
	virtual char getUnifiedCharacter(char originalCharacter)=0;
	virtual short getBinID (char alphabetChar)=0;
	virtual void setSentinel()=0;
protected:
	char unifiedAlphabet[128];
	short alphabetToBinID[128];
};

#endif


