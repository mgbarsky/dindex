#ifndef ORDERBUFFER_H
#define ORDERBUFFER_H
#include "Utils.h"
#include "OrderCell.h"
#include "DNALineReader.h"

//this buffer is a read-write buffer
//	one buffer per bin	 
class OrderStateReadWriteBuffer
{
public:
	OrderStateReadWriteBuffer() ;
	OrderStateReadWriteBuffer(const OrderStateReadWriteBuffer& a) {}
    ~OrderStateReadWriteBuffer();

	bool init(uint64 maxCapacity,  unsigned char binID, const std::string &dbDir, const std::string &tempDir); //memory allocation
	int updateNextOrderCell (OrderCell *resolvingCell, OrderCell *resultingCell); 

	bool reset();
	
    int finishBuffer ();
    void setTotalElementsInBin (int64 val) {this->totalElementsInBin = val;}
private:
    int flushCopyBuffer(); //tbd temporary copy all processed cells
	int flushRefillBuffer();
	unsigned char binID;
	uint64 capacity;
	InputReader *reader;

    int64 totalElementsInBin;
    int64 currentElementInBin;

	int64 totalElementsInBuffer;
	int64 currPositionInBuffer;
	bool firstInBin;
   
	FILE *inputFile;
    FILE *outputFile;
	std::string inputFileName;
	std::string outputFileName;

	OrderCell *buffer;
	
	int currentInputSuffixID;
	std::string inputSuffix;
	std::string outputSuffix;
	
    std::string tempDir;
    std::string dbDir;
};

#endif
