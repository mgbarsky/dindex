#include "MergeManager.h"

using namespace std;

//reads input parameters, allocates buffers
bool MergeManager::init(IndexConfig *cfg, InputReader *reader, short totalChunks)
{
	this->reader = reader;
	this->reader->setSentinel();
	this->totalBins = reader->getAlphabetSize()+1; //+1 for sentinel character which is supposed to be less than any other characater
    this->totalChunks=totalChunks;
	
	this->bwtBuffers.resize(this->totalChunks);
	
	//first need to compute appropriate sizes of each buffer
	uint64 totalMemForMergeBuffers = 1000000000; //default 1 GB
	if(cfg->keyExists("totalMemForMergeBuffers"))	
		totalMemForMergeBuffers = cfg->getValue<uint64>("totalMemForMergeBuffers");	
	else
		displayWarning("MergeManager: total memory for merge is not defined in indexconfig. Using default 1GB");


	this->dataDir =".";
	if(cfg->keyExists("metaDataDir"))
	{
		this->dataDir= cfg->getValue<string>("metaDataDir");
	}

	this->tempDir =".";
	if(cfg->keyExists("tempDir"))
	{
		this->tempDir = cfg->getValue<string>("tempDir");
	}

	string bwtFileExt =".bwt";
	if(cfg->keyExists("bwtFileExt"))
	{
		bwtFileExt = cfg->getValue<string>("bwtFileExt");
	}    

    
    //calculate max memory per each buffer
    //we need totalChunks + totalBins +2 buffers
    uint64 memPerBuffer = totalMemForMergeBuffers /(this->totalChunks + this->totalBins + 2);
    
	//init BWT buffers from temp directory - each chunk has some number of associated files
	for(short i=0;i<this->totalChunks;i++)
	{
		//computes totals per each character in bwt files for each chunk and initializes temp dir
		if( ! this->bwtBuffers[i].init(memPerBuffer,  reader, i, this->dataDir,	this->tempDir,bwtFileExt ))
			return false;
	}		

	//now we need to write the initial files for OrderState input

	//first we are counting total occurrences of each letter of alphabet
	//then, from these counts we initialize our order bins and write them to files
	this->lfTable = new uint64 [this->totalBins];
	for(short j=0; j<this->totalBins; j++)
		this->lfTable[j] = 0;

	for(short i=0;i<this->totalChunks;i++)
	{
		for(short j=0; j<this->totalBins; j++)
		{
			this->lfTable[j] += this->bwtBuffers[i].getLetterBinContribution(j);
		}
	}

	OrderStateOutputBuffer outputBuffer;

	if(!outputBuffer.init((memPerBuffer / sizeof(OrderCell)), this->totalBins, this->tempDir, ".0"))
		return false;

	//distribute bwt characters from each chunk to corresponding bins
	this->totalProcessed=0;
	for(short currentBinID=0; currentBinID < this->totalBins; currentBinID++)
	{
		outputBuffer.initBin(currentBinID);

		for(short currentChunkID=0; currentChunkID<this->totalChunks; currentChunkID++)
		{
			uint64 letterCount = this->bwtBuffers[currentChunkID].getLetterBinContribution(currentBinID);
			for ( uint64 m=0; m < letterCount; m++)
			{
				this->totalProcessed++;
				if(!outputBuffer.initNextCell(currentChunkID))
					return false;
			}
		}
		
		//finished corresponding bin - close file 
		if(!outputBuffer.finishCurrentBin())
			return false;		
	}

	//one input order buffer: memory allocation
	if(!this->inputOrderBuffer.init( (memPerBuffer / sizeof(OrderCell)), this->totalBins,
		this->dataDir, this->tempDir)) //one input buffer for all char bins
		return false;

	//totalBins output order buffers - memory allocation
	this->outputOrderBins.resize(this->totalBins);

	for(short i=0;i<this->totalBins;i++)
	{
		if(!this->outputOrderBins[i].init( (memPerBuffer / sizeof(OrderCell)),  (unsigned char) i, 
		    this->dataDir, this->tempDir))
			return false;
        this->outputOrderBins[i].setTotalElementsInBin(this->lfTable[i]);
        printf("Total in bin %d = %ld\n",i,(long)this->lfTable[i]);
	}
   
	//at this point we have all buffers allocated - memory did not fail 	
	return true;
}


bool MergeManager::mergeChunks()
{	
	//return true;

    short iteration = 1;

	this->totalResolved=0;
     printf("Total to process %ld\n",(long)this->totalProcessed);
	while (this->totalResolved < this->totalProcessed)
	{		
        //reset input buffer
	    if(!this->inputOrderBuffer.reset())
		    return false;
        
	    //reset output read-write buffers
	    for (short i=0; i<this->totalBins; i++)
	    {
		    if (!this->isBinEmpty(i))
		    {
			    if (!this->outputOrderBins[i].reset())
				    return false;
                
		    }
	    }

	    //reset BWT buffers 
	    for(short i=0;i<this->totalChunks;i++)
	    {
		    if( ! this->bwtBuffers[i].reset())
			    return false;
            
	    }

        if (!this->resolveLCP(iteration))
			return false;
        
		iteration++;		
	}
    
	return true;
}


//main logic of one iteration
bool MergeManager::resolveLCP(short iteration)
{
	OrderCell resolvingCell;
	
	this->totalResolved=0;
	int nextBWTSuccessfullyRetrieved = RES_SUCCESS;

    //read next entry of input order array (from prev iteration)
	while(this->inputOrderBuffer.getNextOrderCell(&resolvingCell) == RES_SUCCESS )
	{
		short chunkID = resolvingCell.getChunkID();  

		nextBWTSuccessfullyRetrieved = this->bwtBuffers[chunkID].next(); 
		if (nextBWTSuccessfullyRetrieved != RES_SUCCESS)
		{
			displayWarning("MergeManager: iteration "+T_to_string<short>(iteration) + " of merge failed: logical error - try to access bwt char from chunk when all have been processed");
			return false;
		}

		//get a BWT char from a corresponding BWT buffer
		char bwtChar = this->bwtBuffers[chunkID].getCurrentElement();		
		
		//if BWT cell is resolved for input - skip this cell as input
        if (bwtChar < 0) //skip
        {
            this->totalResolved++;
        }
        else
		{
			short binID = this->reader->getBinID(bwtChar);
			if (binID < 0 || binID >= this->totalBins)	
			{
				displayWarning("MergeManager: iteration "+T_to_string<short>(iteration) + " of merge failed: logical error - invalid bin ID");
				return false;
			}

			OrderCell resultingCell;
			int updateResult = this->outputOrderBins[binID].updateNextOrderCell(&resolvingCell, &resultingCell);
			if(updateResult == RES_SUCCESS)
			{
				if(resultingCell.getState() == STATE_OUTPUT_RESOLVED)
					this->bwtBuffers[chunkID].setCurrentElementProcessed();				
			}
			else if(updateResult == RES_PROCESSED) //logical error - all entries in this bin are processed
			{
				displayWarning("MergeManager: iteration "+T_to_string<short>(iteration) + " of merge failed: logical error - tried to update order cell where all files have been processed");
					return false;
			}
			else //error
				return false;
		}
			
	}

	//finished all bins - flush what remains in each output buffer bin	
	for(short i=0; i<this->totalBins; i++)
	{
		if(!this->isBinEmpty(i))
		{
			if(this->outputOrderBins[i].finishBuffer () != RES_PROCESSED)
				return false;
		}
	}

	//finished all chunk BWTs - flush what remains in each BWT bin
	for(short i=0;i<this->totalChunks;i++)
	{
		if( ! this->bwtBuffers[i].flushWriteOnly())
			return false;
	}

	long resolved = (long)this->totalResolved;
	long processed = (long)this->totalProcessed;
	printf("After iteration %d - %ld entries out of total %ld are resolved\n", iteration, resolved, processed);
	return true;
}

bool MergeManager::isBinEmpty(short binID)
{
	if(this->lfTable[binID]==0)
		return true;
	return false;
}
