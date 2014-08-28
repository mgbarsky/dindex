#ifndef DNALINEREADER_H
#define DNALINEREADER_H

#include "DNAReader.h"

class DNALineReader: public DNAReader
{
public:
	DNALineReader();	
	DNALineReader(DNALineReader& a) {}
	~DNALineReader(){}
	bool init(const std::string &fileName, uint64 offsetStart = 0);
	std::string nextValidToken(uint64 *currentFilePosition);
    void closeFile();

private:
	std::string fileName;
	uint64 fileOffset;
	uint64 currFilePositionBytes;
	uint64 totalBytes;
	std::ifstream file;
};

#endif
