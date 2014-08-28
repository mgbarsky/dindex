#ifndef MERGEMANAGER_H
#define MERGEMANAGER_H

#include "ChunkIndexer.h"
#include "BWTStateBuffer.h"
#include "OrderStateInputBuffer.h"
#include "OrderStateReadWriteBuffer.h"
#include "OrderStateOutputBuffer.h"

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
		delete [] this->lfTable;
		
		std::vector<BWTStateBuffer>().swap(this->bwtBuffers);
		
		std::vector<OrderStateReadWriteBuffer>().swap(this->outputOrderBins);
	}

	bool init(IndexConfig *cfg, InputReader *reader, short totalChunks);
	bool mergeChunks();

private:
	InputReader *reader;
	std::vector<BWTStateBuffer> bwtBuffers;
	std::vector<OrderStateReadWriteBuffer> outputOrderBins;
	OrderStateInputBuffer inputOrderBuffer;
	std::string dataDir;
	std::string tempDir;
	short totalChunks;   
	short totalBins;
	
	uint64 *lfTable;	

	bool resolveLCP(short iteration);
	bool isBinEmpty(short binID);
	uint64 totalResolved;
	uint64 totalProcessed;
};

#endif
