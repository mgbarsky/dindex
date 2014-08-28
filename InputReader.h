#ifndef INPUTREADER_H
#define INPUTREADER_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include "Utils.h"
#include "Alphabet.h"

class InputReader 
{
	public:
		InputReader();

		virtual bool init(const std::string &fileName, uint64 offsetStart = 0)=0;
		virtual std::string nextValidToken(uint64 *currentFileOffset)=0;
        
    virtual void closeFile()=0;
		bool isProcessed ()	{return this->processed;} 
		void setProcessed(bool val) {this->processed=val;}
		char getSentinel(); //returns a character one less than first valid alphabet char - to sort properly
		short getAlphabetSize() {return this->alphabet->getSize();}
		short getBinID (char alphabetChar){return this->alphabet->getBinID(alphabetChar);}
		char getUnifiedCharacter(char originalCharacter){return this->alphabet->getUnifiedCharacter(originalCharacter);}
		void setSentinel(){this->alphabet->setSentinel();}
	protected:
		Alphabet *alphabet;
	private:
		bool processed;
};

#endif
