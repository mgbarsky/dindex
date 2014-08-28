#ifndef ORDERSTATEINPUTBUFFER_H
#define ORDERSTATEINPUTBUFFER_H
#include "Utils.h"
#include "DNALineReader.h"
#include "OrderCell.h"

//this buffer is a read-only buffer which processes all bins in order
//file pointer remains open during the processing because it is a single sequential processing
class OrderStateInputBuffer
{
public:
	OrderStateInputBuffer() ;
	OrderStateInputBuffer(const OrderStateInputBuffer& a) {}
    ~OrderStateInputBuffer();

	bool init(uint64 maxCapacity, short totalBins, const std::string &dbDir, const std::string &tempDir); //memory allocation
	int getNextOrderCell(OrderCell *resultCell); //reads cell currently pointed to from buffer, advances pointer, returns 0-failure, 1-success, 2-no more cells to process
	
	bool reset();

private:
	int refillBuffer();
	
	OrderCell *buffer;

	std::string tempDir;
    std::string dbDir;

	short totalBins;

	short currentBinID;
	
	int64 currPositionInBuffer;
	int64 totalElementsInBuffer;

	FILE *file;
	std::string inputFileName;
	
	uint64 capacity;
	int currentInputSuffixID;
	std::string inputSuffix;
};

#endif
