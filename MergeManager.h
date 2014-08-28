#ifndef MERGEMANAGER_H
#define MERGEMANAGER_H

#include "ChunkIndexer.h"
#include "BWTStateBuffer.h"

#include "OrderStateReadWriteBuffer.h"
#include "OrderStateOutputBuffer.h"
#include "InOrderManager.h"

class MergeManager
{
public:    
	MergeManager() 
	{
		this->lfTable=NULL;
		this->totalChunks=0;
		this->totalBins=0;
	}
	MergeManager(MergeManager& a) {}
	~ MergeManager()
	{
		if (this->lfTable != NULL) delete [] this->lfTable;
		
		std::vector<BWTStateBuffer>().swap(this->bwtBuffers);
		
		std::vector<OrderStateReadWriteBuffer>().swap(this->outputOrderBins);
	}

	bool init(IndexConfig *cfg, InputReader *reader, short totalChunks);
	bool mergeChunks();

private:
	InputReader *reader;
	std::vector<BWTStateBuffer> bwtBuffers;
	std::vector<OrderStateReadWriteBuffer> outputOrderBins;
    InOrderManager inOrder;
	
	std::string dataDir;
	std::string tempDir;
	short totalChunks;   
	short totalBins;
	
	uint64 *lfTable;	

	bool nextIteration (short iteration);
	bool resolveLCP (short iteration);
    bool nextBifurcation ();
	//bool isBinEmpty(short binID);

    //uint64 currentPositionCounter;
	uint64 totalToProcess;
	//uint64 totalResolved;
	//uint64 totalProcessed;
};

#endif
