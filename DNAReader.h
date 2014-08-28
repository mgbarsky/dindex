#ifndef DNAREADER_H
#define DNAREADER_H

#include "InputReader.h"
#include "DNAAlphabet.h"

class DNAReader: public InputReader
{
public:
	DNAReader();
	DNAReader(DNAReader& a) {}
	~ DNAReader()
	{
		delete this->alphabet;
	}
	
	//this to be implemented in each inheriting reader
	virtual bool init(const std::string &fileName, uint64 offsetStart = 0)=0;
	virtual std::string nextValidToken(uint64 *currentFileOffset)=0;
    virtual void closeFile()=0;
	void setSentinel();
protected:	
	bool validLine(const std::string &line);
	
private:	
	void initAlphabet();
	char validAlphabet[128];	
};

#endif
