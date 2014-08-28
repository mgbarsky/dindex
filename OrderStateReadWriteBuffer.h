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
	OrderStateReadWriteBuffer(const OrderStateReadWriteBuffer& a) {this->debug = false;}
    ~OrderStateReadWriteBuffer();

	bool init(uint64 maxCapacity,  unsigned char binID, const std::string &dbDir, const std::string &tempDir, int64 totalElements); //memory allocation
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
    int currentOutputSuffixID;
	std::string inputSuffix;
	std::string outputSuffix;

    std::vector <int64> stateCounters[2]; //1 for input 1 for output, interchangeable
	
    int64 inIntervalRtotal; //count of resolbed in the current input interval
    int64 inCurrentRpos;  //current position within inInterval of resolved

    int64 inIntervalURtotal; //count of unresolved in the currrent input interval
    int64 inCurrentURpos; //position in current unresolved inteval

   // int64 outCurrentRtotal; //total count of new consecutive resolved cells
   // int64 outCurrentURtotal; //total count of new consecutive unresolved cells
   // int64 currPosOutState;

    std::vector<int64>::iterator currStateCounterIt;

    std::string tempDir;
    std::string dbDir;

    bool debug;
};

#endif
