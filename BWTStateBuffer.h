#ifndef BWTSTATEBUFFER_H
#define BWTSTATEBUFFER_H

#include "Utils.h"
#include<stdio.h>
#include <string>
#include "InputReader.h"

#define ITERATION_STATE_UPDATE_MODE 0
#define BIFURCATION_FILTER_MODE 1

//one buffer per chunk
//In this buffer file pointer is always stays closed, and reopens when needed
//this is because there could be too many chunks to have 1 fp per chunk
class BWTStateBuffer
{
public:
	BWTStateBuffer();
	~BWTStateBuffer();
	BWTStateBuffer(const BWTStateBuffer& a) {}; // copy constructor not allowed
	bool init( uint64 maxBufferCapacityBytes,  InputReader *reader, short chunkID, std::string const &dbDir,
		 std::string const &tempDir,  std::string const &bwtFileExt);
	char getCurrentCell();
	void setCurrentCell(char newCell);

    int getNextCell(char *resultCell);

	int next(); //advances to the next element in chunk BWT
	uint64 getLetterBinContribution(unsigned short binID); //returns total count of the corresponding character in this chunk
	
	bool nextIterationReset();	
    bool nextBifurcationReset();	

    //public because they are also called at the end to flush what remains in buffer
	bool flush(); //flushes content of a buffer to an output file
    bool flushUnresolved(); //flushes only unresolved (positive) cells to an output file 

private:	
	short chunkID;
	int refill(); //reads chunk file into a buffer 
	
	uint64 *lfTable;	

	unsigned short totalLetterBins;  //how many different characters in the alphabet	

	uint64 currPositionInFile; 

	int64 currPositionInBuffer;
	int64 totalElementsRead;

	char *buffer;

	FILE *inputFile;
	FILE *outputFile;
    std::string fileName0;
    std::string fileName1;	
	
    std::string inputFileName;
    std::string outputFilename;	
	uint64 capacity;

    int mode;
};

#endif
