#ifndef DNAALPHABET_H
#define DNAALPHABET_H

#include "Alphabet.h"

class DNAAlphabet : public Alphabet
{
public:
	DNAAlphabet();
	~DNAAlphabet() {}
	DNAAlphabet(DNAAlphabet& a) {}; // copy constructor not allowed
	char getSentinel();
	short getSize();
	char getUnifiedCharacter(char originalCharacter);
	short getBinID (char alphabetChar);
	void setSentinel();
};

#endif