#ifndef ORDERSTATEOUTPUTBUFFER_H
#define ORDERSTATEOUTPUTBUFFER_H
#include "Utils.h"
#include "DNALineReader.h"
#include "OrderCell.h"

//this buffer is used when initializing order arrays
//it initializes bin cells and writes them into corresponding bin file in temp directory
//one buffer for all bins
//file pointer remains open when working with the current bin
class OrderStateOutputBuffer
{
public:
	OrderStateOutputBuffer() ;
	OrderStateOutputBuffer(const OrderStateOutputBuffer& a) {}
    ~OrderStateOutputBuffer();

	bool init(uint64 maxCapacity, short totalBins, const std::string &tempDir, const std::string &outputFileSuffix); //memory allocation

	bool initBin (short binID);

    bool finishCurrentBin();

	bool initNextCell(short chunkID);

	

private:
	OrderCell *buffer;
	
	std::string tempDir;
	std::string outputFileSuffix;
	std::string outputFilePrefix;

	short currentBinID;
	short totalBins;
	std::string outputFileName;
	FILE *file;

	uint64 currPositionInBuffer;
    
	uint64 capacity;
	bool firstInBin;

    bool flushBuffer();
};

#endif
