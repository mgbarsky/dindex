#ifndef BWTSTATEBUFFER_H
#define BWTSTATEBUFFER_H

#include "Utils.h"
#include<stdio.h>
#include <string>
#include "InputReader.h"

//one buffer per chunk
//In this buffer file pointer is always stays closed, and reopens when needed
//this is because there could be too many chunks to have 1 fp per chunk
class BWTStateBuffer
{
public:
	BWTStateBuffer();
	~BWTStateBuffer();
	BWTStateBuffer(const BWTStateBuffer& a) {}; // copy constructor not allowed
	bool init( uint64 maxBufferCapacity,  InputReader *reader, short chunkID, std::string const &dbDir,
		 std::string const &tempDir,  std::string const &bwtFileExt);
	char getCurrentElement();
	void setCurrentElementProcessed();

	int next(); //advances to the next element in chunk BWT
	uint64 getLetterBinContribution(unsigned short binID); //returns total count of the corresponding character in this chunk
	
	bool reset();	
	bool toogleInputOutput();

	bool flushWriteOnly(); //called at the end to flush what remains in buffer

private:	
	short chunkID;
	int refillReadOnly(); //reads chunk file into a buffer for the first time
	
	int flushRefill(); //writes what is in buffer, reads remaining records
	
	uint64 *lfTable;	

	unsigned short totalLetterBins;  //how many different characters in the alphabet
	
	std::string inputSuffix;
	std::string outputSuffix;
	int currentInputSuffixID;

	uint64 currPositionInFile; 

	int64 curPositionInBuffer;
	int64 totalElementsInBuffer;

	char *buffer;

	FILE *inputFile;
	FILE *outputFile;
	std::string tempDir;
	std::string dbDir;

	std::string bwtFileExt;
	std::string inputFileName;	
	std::string outputFileName;	
	uint64 capacity;
};

#endif
