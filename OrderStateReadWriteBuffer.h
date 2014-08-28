#ifndef ORDERBUFFER_H
#define ORDERBUFFER_H
#include "Utils.h"
#include "OrderCell.h"
#include "InOrderCell.h"
#include "DNALineReader.h"

//this buffer is a read-write buffer
//	one buffer per bin
//once file is opened for reading or for writing, it remains open until all is processed	 
class OrderStateReadWriteBuffer
{
public:
	OrderStateReadWriteBuffer() ;
	OrderStateReadWriteBuffer (const OrderStateReadWriteBuffer& a) {this->debug = false;}
    ~OrderStateReadWriteBuffer();

	bool init(uint64 totalMemInBytes,  unsigned char binID, const std::string &tempDir, int64 totalElementsInBin); //memory allocation
	int updateNextOrderCell (InOrderCell *resolvingCell, char *finalState); 
    int getNextCell (OrderCell *resultingCell);

	bool nextIterationReset();	
    bool nextBifurcationReset();
	
    bool wrapUpIteration ();
    bool wrapUpBifurcation ();
    void wrapUp (); //closes open final files at the end of the program
    void setTotalElementsInBin (uint64 total) { this->totalElementsInBin = total; }
private:
    bool flush(); 
    bool flushBifurcation();
	int refill(); //refills buffer starting from pos 0 to upto capacity
    int flushRefill(); //complex function which leaves last element in buffer - during iteration
    int refillBifurcation(); //the difference is that we do not leave last element in buffer

	unsigned char binID;
	uint64 capacity;
	InputReader *reader;

    int64 totalElementsInBin;
    int64 currentElementInBin;

	int64 totalElementsInBuffer;
	int64 currPositionInBuffer;
	   
	FILE *inputFile;
    FILE *outputFile;

    FILE *finalHeaderFile;
    FILE *finalOutputFile;

    std::string fileName0; //reads from it during iteration, writes to it during bifurcation
	std::string fileName1; //writes to it during iteration, reads from it during bifurcation

	//std::string inputFileName;
	//std::string outputFileName;

	OrderCell *buffer;
	
	int currentInputSuffixID;
    int currentOutputSuffixID;
	//std::string inputSuffix;
	//std::string outputSuffix;

    std::vector <int64> stateCounters[2]; //1 for input 1 for output, interchangeable
	
    int64 inIntervalRtotal; //count of resolbed in the current input interval
    int64 inCurrentRpos;  //current position within inInterval of resolved

    int64 inIntervalURtotal; //count of unresolved in the currrent input interval
    int64 inCurrentURpos; //position in current unresolved inteval

   // int64 outCurrentRtotal; //total count of new consecutive resolved cells
   // int64 outCurrentURtotal; //total count of new consecutive unresolved cells
   // int64 currPosOutState;

    std::vector<int64>::iterator currStateCounterIt;

    uint64 currentPosInFinalOutput;

    //std::string tempDir;
    //std::string dbDir;

    bool debug;
};

#endif
