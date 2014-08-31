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
    //we need totalChunks + totalBins +1 buffer for input - TBD add for vectors ?? what about using inputorder as no buffer - buffered by system
    uint64 memPerBuffer = totalMemForMergeBuffers /(this->totalChunks + this->totalBins + 1);
    
	//init BWT buffers from db directory - originally, each chunk has it own bwt in db directory
    //the bwtBuffer BWTStateBuffer will read corresponding chunk, count total counts for each letter
    //and write BWT itself to tempDir - to serve as an input for iteration 1
	for(short i=0;i<this->totalChunks;i++)
	{
		//computes totals per each character in bwt files for each chunk and initializes temp dir
		if( ! this->bwtBuffers[i].init(memPerBuffer,  reader, i, this->dataDir,	this->tempDir,bwtFileExt ))
			return false;
	}		

	//now we need to write the initial files for first iteration input and output

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

    //init inOrder manager
    if (!this->inOrder.init(memPerBuffer, this->tempDir))
        return false;
	OrderStateOutputBuffer outputBuffer;
    
	if(!outputBuffer.init(memPerBuffer , this->totalBins, this->tempDir))
		return false;

	//distribute bwt characters from each chunk to corresponding bins
    //at the same time, write inOrdercells for the first iteration
	this->totalToProcess=0;
    
	for(short currentBinID=0; currentBinID < this->totalBins; currentBinID++)
	{
		outputBuffer.initBin(currentBinID);

		for(short currentChunkID=0; currentChunkID<this->totalChunks; currentChunkID++)
		{
			uint64 letterCount = this->bwtBuffers[currentChunkID].getLetterBinContribution(currentBinID);
			for ( uint64 m=0; m < letterCount; m++)
			{
				this->totalToProcess++;
				if(!outputBuffer.initNextCell(currentChunkID))
					return false;
                
                if (!inOrder.setNextInOrderCell(currentChunkID,(char) currentBinID))
                    return false;
			}
		}
		
		//finished corresponding bin - close file 
		if(!outputBuffer.wrapUpCurrentBin())
			return false;		
	}
    //finished inOrder - wrapUp
    if (!inOrder.wrapUp())
        return false;	

	//totalBins output order buffers - memory allocation
	this->outputOrderBins.resize (this->totalBins);

	for(short i=0;i<this->totalBins;i++)
	{
		if(!this->outputOrderBins[i].init( memPerBuffer,  (unsigned char) i, 
		     this->tempDir, this->lfTable[i] ))
			return false;
        
        printf("Total in bin %d = %ld\n",i,(long)this->lfTable[i]);
	}
   
	//at this point we have all buffers allocated - memory did not fail 	
	return true;
}

bool MergeManager::mergeChunks() {

    short iteration = 1;

	while (this->totalToProcess > 0)	{        
  
        //*************************
        // Next iteration
        //********************************
        if (!this->nextIteration ( iteration))
            return false;

        //***************
        // Next bifurcation: Here where input and output resoved are distributed between input and output for the next iteration
        //*****************
        if (!this->nextBifurcation())
            return false; 
        		
		iteration++;
       
        long remains  = (long)this->totalToProcess;
	    printf("After iteration %d - remains %ld entries to resolve\n", iteration, remains);		
	}
    
    //close files with final results
    for (short i=0; i<this->totalBins; i++)   {	    
	   this->outputOrderBins[i].wrapUp();
    }  
	return true;
}

//one iteration
bool MergeManager::nextIteration(short iteration)
{
	if (iteration == 14)
           int debug =0;
    //reset BWT buffers for current iteration
    for(short i=0;i<this->totalChunks;i++)    {
	    if( ! this->bwtBuffers[i].nextIterationReset())
		    return false;
    }

    //reset output bins for current iteration
    for (short i=0; i<this->totalBins; i++)   {	    
		if (!this->outputOrderBins[i].nextIterationReset())
			return false;
        this->outputOrderBins[i].setTotalElementsInBin(this->lfTable[i]);  
    }

    //reset inOrder for current iteration
    if(!this->inOrder.nextIterationReset())
	    return false;

//************main action
    if (!this->resolveLCP(iteration))
		return false;
//************************

    //wrap up bwt chunks after current iteration - flush what remains in each buffer - full flush
    for(short i=0;i<this->totalChunks;i++)    {
	    if(!this->bwtBuffers[i].flush() )
		    return false;
    }

    //wrap up output bins after current iteration - flush and close file pointers
    for (short i=0; i<this->totalBins; i++)   {	    
	   if(!this->outputOrderBins[i].wrapUpIteration() )
	        return false;	   
    }  

    return true;      	   	    
}

//here the results of the previous iteration get distributed between:
//1. working input order array (input for next iteration - if not resolved for input, which is marked as first bit of bwt
//2. working output order arrays
//
//also bwts get cut off all cells resolved for input
bool MergeManager::nextBifurcation() {
    
    //reset buffers for new processing
    //reset BWT managers for current bifurcation
    for(short i=0;i<this->totalChunks;i++)    {
	    if( ! this->bwtBuffers[i].nextBifurcationReset())
		    return false;
    }

    //reset output bins for current iteration
    for (short i=0; i<this->totalBins; i++)   {	    
	    if (!this->outputOrderBins[i].nextBifurcationReset())
		    return false;  
    }

    //reset inOrder for current iteration
    if(!this->inOrder.nextBifurcationReset())
	    return false;

    //reset lftable counts for each bin, now we are going to count how much remains (unresolved for output)
    this->totalToProcess = 0;
	uint64 outputUnResolvedTotal= 0;
    for(short j=0; j<this->totalBins; j++)
		this->lfTable[j] = 0;

    //read all output bins in turn, 
    //look at the corresponding BWT entry - according to a new order after the last iteration -
    //and depending what you see, distribute out cell: to new output buffer, to new input buffer, to truncated bwts, and (TBD) to final output
    OrderCell outCell;
    for (short i=0; i<this->totalBins; i++) {
        
        int readingResult = this->outputOrderBins[i].getNextCell ( &outCell);
        if ( readingResult == RES_FAILURE ) return false;

        while (readingResult != RES_PROCESSED) {
            //look at current outCell
            short chunkID = outCell.getChunkID();
            
            char bwtChar =0;
            int bwtReadingResult = (this->bwtBuffers[chunkID]).getNextCell(&bwtChar);

            if (bwtReadingResult != RES_SUCCESS) {
                displayWarning ("MergeManager::nextBifurcation Logical error - failed to access BWT entry.");
                return false;
            }
           
            if (bwtChar > 0) {//not resolved for input - write this entry into inOrder  
                if ( !this->inOrder.setNextInOrderCell (outCell.getChunkID(), outCell.getResolvingChar () ) )
				{
                    return false;
					
				}
                
                this->totalToProcess ++;
	        }

            //if out cell not resolved for output - write it into outOrderBin - that is performed automatically by the buffer - due to filter function           
            char outState = outCell.getState();
            if (outState != STATE_OUTPUT_RESOLVED)
			{
                this->lfTable [i] ++;
				outputUnResolvedTotal++;
			}
            //BWT chars will be skipped authomatically by buffer - due to filter function
            readingResult = this->outputOrderBins[i].getNextCell ( &outCell);
            if ( readingResult == RES_FAILURE ) return false;
        }

		
        //finished writing new outCells for current bin - wrap it up
        if(!this->outputOrderBins[i].wrapUpBifurcation() )
	        return false;
    }
	   
    //finished writing new in cells - wrap it up
    if(!this->inOrder.wrapUp() )
        return false;

    //also flush what remains unresolved in bwtBuffers buffers
    for(short i=0;i<this->totalChunks;i++)    {
	    if(!this->bwtBuffers[i].flushUnresolved() )
		    return false;
    }
	if (this->totalToProcess != outputUnResolvedTotal)
		{
			displayWarning ("The total of remaining output "+ T_to_string<uint64>(this->totalToProcess) 
				+" does not correspond to the total of remaining input "+ T_to_string<uint64>(outputUnResolvedTotal) );
			return false;
		}
    return true;
}


bool MergeManager::resolveLCP(short iteration)
{
	uint64 counter=0;
	InOrderCell inOrderCell;	
	
	int nextBWTSuccess = RES_SUCCESS;  //success of advancing pointer in bwt buffer

    //read next entry of input order array (from prev iteration)
    int readingResult = this->inOrder.getNextInOrderCell (&inOrderCell);
    if (readingResult == RES_FAILURE)
        return false;
	while ( readingResult != RES_PROCESSED ) {
		short chunkID = inOrderCell.getChunkID();  

		nextBWTSuccess = this->bwtBuffers[chunkID].next(); 
		if (nextBWTSuccess != RES_SUCCESS) {
			displayWarning("MergeOrderManager: iteration "+T_to_string<short>(iteration) + " of merge failed: logical error - try to access bwt char from chunk when all have been processed");
			return false;
		}

		//get a BWT char from a corresponding BWT buffer
		char bwtChar = (this->bwtBuffers[chunkID]).getCurrentCell();			
		short binID = this->reader->getBinID (bwtChar);
		if (binID < 0 || binID >= this->totalBins)		{
			displayWarning("MergeOrderManager: iteration "+T_to_string<short>(iteration) + " of merge failed: logical error - invalid bin ID");
			return false;
		}

		char finalState;	
		int updateResult = this->outputOrderBins[binID].updateNextOrderCell (&inOrderCell, &finalState);
		if(updateResult == RES_SUCCESS)	{           
			if(finalState == STATE_OUTPUT_RESOLVED)  {
				bwtChar = - bwtChar;	//set bit to processed	
                this->bwtBuffers [chunkID].setCurrentCell (bwtChar);
            }
		}
		else if(updateResult == RES_PROCESSED) {//logical error - all entries in this bin are processed		
			displayWarning("MergeOrderManager: iteration "+T_to_string<short>(iteration) + " of merge failed: logical error - tried to update order cell where all files have been processed");
				return false;
		}
		else //some unexpected error
			return false;
        readingResult = this->inOrder.getNextInOrderCell (&inOrderCell);
        if (readingResult == RES_FAILURE)
            return false;
		counter++;
	}
	
	return true;
}

