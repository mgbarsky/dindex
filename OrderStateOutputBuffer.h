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

	bool init(uint64 maxCapacity, short totalBins, const std::string &tempDir); //memory allocation

	bool initBin (short binID);

    bool wrapUpCurrentBin();

	bool initNextCell(short chunkID);

	

private:
	OrderCell *buffer;
	
	std::string tempDir;

	short currentBinID;
	short totalBins;
	std::string fileName;
	FILE *file;

	uint64 currPositionInBuffer;
    
	uint64 capacity;
	bool firstInBin;

    bool flush();
};

#endif
