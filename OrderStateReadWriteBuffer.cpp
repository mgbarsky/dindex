#include "OrderStateReadWriteBuffer.h"

using namespace std;
void printVector (const std::vector<int64> &vec)
{
    int i=0;
    for (;i<vec.size();i++)
        printf("%ld ",(long)(vec[i]));

    printf("\n");
}
OrderStateReadWriteBuffer::OrderStateReadWriteBuffer() 
{
	this->buffer = NULL;	
    this->inputFile = NULL;
    this->outputFile = NULL;
    this->debug = false;
}
	
OrderStateReadWriteBuffer::~OrderStateReadWriteBuffer()
{
	delete [] this->buffer;
}

//memory allocation - occurs once per program
bool OrderStateReadWriteBuffer::init( uint64 maxCapacity, unsigned char binID, 
		const std::string &dbDir,  const std::string &tempDir, int64 totalElements)
{
	this->capacity = maxCapacity; 
	this->binID = binID;
	
    this->dbDir = dbDir;
	this->tempDir=tempDir;

   	this->buffer = new OrderCell[(size_t)this->capacity];

    this->totalElementsInBin =totalElements;

    //initialize input state counter
    this->stateCounters[0].push_back (totalElements);

    this->currentElementInBin =0;
    if(!this->buffer)
    {
        displayWarning("OrderStateReadWriteBuffer: buffer memory allocation for "+T_to_string<int64>(this->capacity)+" elements failed");
        return false;
    }

	this->totalElementsInBuffer=0;
	this->currPositionInBuffer=-1; //not initialized yet
	
	this->currentInputSuffixID=1; //will switch once reset is called
	
	return true;
}

bool OrderStateReadWriteBuffer::reset()
{
    if (this->debug) {
    printf("\n NEW ITERATION for bin %d------------------------\n",(int)binID);
    printf("Input state vector for current iteration:\n");
            printVector (this->stateCounters[this->currentInputSuffixID]);
            printf("Output state vector for current iteration:\n");
            printVector (this->stateCounters[this->currentOutputSuffixID]);
    printf("------------------------\n");
}
	if(this->currentInputSuffixID == 1)
	{
		this->currentInputSuffixID=0;
        this->currentOutputSuffixID = 1;
		this->inputSuffix=".0";
		this->outputSuffix=".1";
        this->stateCounters[1].clear(); //prepare output vector for the next iteration

	}
	else
	{
		this->currentInputSuffixID=1;
        this->currentOutputSuffixID = 0;
		this->inputSuffix=".1";
		this->outputSuffix=".0";
        this->stateCounters[0].clear(); //prepare output vector for the next iteration
	}

	this->currPositionInBuffer=-1;
	this->totalElementsInBuffer=0;
	
    this->currentElementInBin =0;

    //reset all interval variables to zero
    this->inIntervalRtotal=0; 
    this->inCurrentRpos=0;  

    this->inIntervalURtotal=0; 
    this->inCurrentURpos=0; 

   // this->outCurrentRtotal=0; 
   // this->outCurrentURtotal=0; 

    this->currStateCounterIt = this->stateCounters[currentInputSuffixID].begin();

    //currPosOutState=0;

	//open input file and output file
    this->inputFileName = this->tempDir +separator()+ "orderarray"
				+ T_to_string<short>(this->binID)+this->inputSuffix;
    this->inputFile = fopen (this->inputFileName.c_str(),"rb");	

	if(!this->inputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - reset failed: File "+this->inputFileName+" cannot be found.");
		return false;
	}

    this->outputFileName = this->tempDir +separator()+ "orderarray"
				+ T_to_string<short>(this->binID)+this->outputSuffix;
    this->outputFile = fopen (this->outputFileName.c_str(),"wb");	

	if(!this->outputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - reset failed: cannot write to file "+this->inputFileName+".");
		return false;
	}

	return true;
}

int OrderStateReadWriteBuffer::finishBuffer ()
{
    int ret = this->flushCopyBuffer();

    if (ret != RES_PROCESSED)  //error occured
        return ret;

    fclose (this->inputFile);
    this->inputFile = NULL;

    fclose (this->outputFile);
    this->outputFile = NULL;
    
    //copy the remaining entry from input state to output state - the only one which may happen is resolved tail
    if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
         return RES_PROCESSED;
    int64 lastInVal = *this->currStateCounterIt;

    if (lastInVal >0)
    {
        displayWarning("OrderStateReadWriteBuffer - unprocessed values remain in input state vector.");
		return RES_FAILURE;
    }   
    
    //copy this value to the end of output state vector
    size_t totalInOutput = this->stateCounters[this->currentOutputSuffixID].size();
    int64 lastVal = this->stateCounters[this->currentOutputSuffixID][totalInOutput-1];
    if (lastVal < 0 ) //add total remaining resolved to it
    {
        this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]+=lastInVal;
    }
    else //last value in vector - unresolved interval
    {
        //start new resolved interval
        this->stateCounters[this->currentOutputSuffixID].push_back(lastInVal);              
    }
    return RES_PROCESSED;
}

//this updates current cell in buffer using the information in the same cell from previous iteration and resolving oprder cell
//it returns the result: 0 - failed, 1-success, 2-no more cells to update
int OrderStateReadWriteBuffer::updateNextOrderCell (OrderCell *resolvingCell, OrderCell *resultingCell) 
{
	if(this->currPositionInBuffer ==-1) {  //read first value from input state buffer
        //check that not after end 
        if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
        {
            displayWarning("OrderStateReadWriteBuffer - invalid position in state interval vector.");
		    return RES_FAILURE;
        }
        int64 val = *(this->currStateCounterIt);
        if (val > 0 ) {  //unresolved interval
            this->inIntervalRtotal=0; 
            this->inCurrentRpos=0;  

            this->inIntervalURtotal=val; 
            this->inCurrentURpos=0; 
        }
        else {  //resolved interval
            this->inIntervalRtotal= - val; 
            this->inCurrentRpos=0;  

            this->inIntervalURtotal=0; 
            this->inCurrentURpos=0; 
        } 
        this->currStateCounterIt ++;       
    }

    
    if(this->currPositionInBuffer ==-1 || 
		this->currPositionInBuffer == this->totalElementsInBuffer)  //need to flush data and to refill new data
	{	
		int ret = this->flushRefillBuffer();
		if (ret != RES_SUCCESS)
			return ret;
	}

    //default state - the cell is in the middle of the buffer array
    bool prevCellAvailable = true;
    bool nextCellAvailable = true;
    bool lastInBin = false;
    bool nextCellResolved = false;
    bool prevCellResolved = false;
    
    if (this->currPositionInBuffer == 0) //there is no previous cell in buffer to look at
        prevCellAvailable = false;

    if  (this->currentElementInBin == this->totalElementsInBin - 1) //last cell in the entire bin
        lastInBin = true;

    //now about the next cell
    if (this->currPositionInBuffer == this->totalElementsInBuffer - 1) //last cell of this buffer
        nextCellAvailable = false;   

//skip resolved cells - will not be here in the final version
    while(this->buffer[this->currPositionInBuffer].getState () == STATE_OUTPUT_RESOLVED) //continue increment counter - this copies prev value to the output buffer
	{
          this->inCurrentRpos++;
        //check that state counters are correct 
        if (  this->inIntervalRtotal == 0 ) { //we could not get here if current interval represents unresolved cells
            printf("Input state vector for current iteration:\n");
            printVector (this->stateCounters[this->currentInputSuffixID]);
            printf("Output state vector for current iteration:\n");
            printVector (this->stateCounters[this->currentOutputSuffixID]);

            printf ("current pos in bin = %ld, current pos in buffer=%ld, curr position in interval = %ld\n",
                (long)(this->currentElementInBin), (long)this->currPositionInBuffer, (long) this->inCurrentRpos);
            displayWarning("OrderStateReadWriteBuffer - encountered resolved cell where unresolved has been expected.");
		    return RES_FAILURE;
        } 

        //reached the end of the resolved interval
        if (this->inCurrentRpos == this->inIntervalRtotal ) //switch state
        { 
            //record total resolved to the last position of the output state array
            size_t totalInOutput = this->stateCounters[this->currentOutputSuffixID].size();
            if (totalInOutput == 0)
                this->stateCounters[this->currentOutputSuffixID].push_back(-this->inCurrentRpos); 
            else
            {
                int64 lastVal = this->stateCounters[this->currentOutputSuffixID][totalInOutput-1];
                if (lastVal < 0 ) //add total resolved to it
                {
                    this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]-=this->inCurrentRpos;
                }
                else //last value in vector - unresolved interval
                {
                    //start new resolved interval
                    this->stateCounters[this->currentOutputSuffixID].push_back(-this->inCurrentRpos);              
                }
            }
            //get next value from input state, it should be positive
            //check that not after end 
            if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
            {
                displayWarning("OrderStateReadWriteBuffer - invalid position in state interval vector.");
		        return RES_FAILURE;
            }
            int64 val = *(this->currStateCounterIt);
            if (val > 0 ) {  //unresolved interval
                this->inIntervalRtotal=0; 
                this->inCurrentRpos=0;  

                this->inIntervalURtotal=val; 
                this->inCurrentURpos=0; 
            }
            else {  //resolved interval, comes after resolved interval - error
                displayWarning("OrderStateReadWriteBuffer - resolved comes after resolved in state interval vector.");
		        return RES_FAILURE;
            } 
            
            this->currStateCounterIt ++; 
           
        }
		this->currPositionInBuffer++;
        this->currentElementInBin++;
       
		if(	this->currPositionInBuffer == this->totalElementsInBuffer)  //need to flush data and to reset
		{	
			int ret = this->flushRefillBuffer();
			if (ret!=RES_SUCCESS)
				return ret;
		}       
       
	}

    //check that resolved count corresponds to the actual cells skipped
    if ( this->inIntervalRtotal > 0) //incorrect state
    {
         displayWarning("OrderStateReadWriteBuffer - reached incorrect state (resolved) during next cell update.");
		 return RES_FAILURE;
    }

    if (this->inIntervalURtotal == 0) //incorrect state
    {
         displayWarning("OrderStateReadWriteBuffer - reached incorrect state (resolved) during next cell update.");
		 return RES_FAILURE;
    }

    //we now are staying on the current cell which is not yet fully STATE_OUTPUT_RESOLVED

    //if this is the first cell of the current unresolved interval - there is no previous cell available
    if (this->inCurrentURpos == 0)
    {
        prevCellResolved = true;
        prevCellAvailable=false; //there may be prev cell in buffer but this would not be the correct cell
    }

    //if this is the last cell of the current unresolved interval - there is no next cell, and the cell is not required to be carried to the next iteration if it is resolved for the prev
    if (this->inCurrentURpos == this->inIntervalURtotal -1)
    {
        nextCellResolved = true;
        nextCellAvailable = false; 
    }

    
//***********************
//START UPDATING CURRENT CELL
//*********************

    //the only things that can change are chunkID and we need to carry on resolving char - we update these
	this->buffer[this->currPositionInBuffer].setResolvingChar (resolvingCell->getResolvingChar());
    this->buffer[this->currPositionInBuffer].setChunkID (resolvingCell->getChunkID());
   

    //its previous state can be STATE_RESOLVED_RIGHT_LEFT
    //in this case, just upgrade it to STATE_OUTPUT_RESOLVED
    if (this->buffer[this->currPositionInBuffer].getState () == STATE_RESOLVED_RIGHT_LEFT)
	{
		this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
	}
    //if its previous state is resolved left - look at the next cell if available and upgrade to resolved if next cell does not need current cell anymore
    else if (this->buffer[this->currPositionInBuffer].getState () == STATE_RESOLVED_LEFT)
    {
        if (this->buffer[this->currPositionInBuffer].getResolvingChar() == 0) //end of suffix
		{
			this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED) ;
		}
        else
        {
            //check next cell if available
			if(nextCellAvailable) //not the last cell of the buffer
			{
				if(this->buffer[this->currPositionInBuffer+1].getState () >= STATE_RESOLVED_LEFT)
				{
					this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
				}
			}
			else if (nextCellResolved) //next cell is resolved - do not need current cell anymore
			{
				this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
			}
            else if (lastInBin) //last element of a bin order array
            {
                this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
            }
        }
    }	
	else //state is unresolved - try to resolve it
	{
		if(this->buffer[this->currPositionInBuffer].getResolvingChar () == 0) //end of suffix
		{
			this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
		}
		else
		{
            //look into previous cell if available
            if (!prevCellAvailable)
            {
                displayWarning("Logical error - no previous cell to compare");
				return RES_FAILURE;
            }
			if(this->buffer[this->currPositionInBuffer-1].getResolvingChar () == this->buffer[this->currPositionInBuffer].getResolvingChar ())
				this->buffer[this->currPositionInBuffer].incrementLCP ();
			else
			{
				this->buffer[this->currPositionInBuffer].setState (STATE_RESOLVED_LEFT);
				this->buffer[this->currPositionInBuffer].setSplitCharsBeforeAfter (this->buffer[this->currPositionInBuffer-1].getResolvingChar() ,
                                            this->buffer[this->currPositionInBuffer].getResolvingChar() );                
			}
		}		
   
        //after split char comparison, state of a current cell can change to resolved left - 
        if(this->buffer[this->currPositionInBuffer].getState () == STATE_RESOLVED_LEFT) 
        {
            //check next cell if available to upgrade to resolved
            if(nextCellAvailable) //not the last cell of the buffer
			{
				if(this->buffer[this->currPositionInBuffer+1].getState () >= STATE_RESOLVED_LEFT)
				{
					this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
				}
			}
			else if (nextCellResolved) //next cell is resolved - do not need current cell anymore
			{
				this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
			}
            else if (lastInBin) //last element of a bin order array
            {
                this->buffer[this->currPositionInBuffer].setState (STATE_OUTPUT_RESOLVED);
            }
        }           
    

//if current state changed to resolved - we need to see if we can upgrade previous cell
        if (this->buffer[this->currPositionInBuffer].getState () == STATE_RESOLVED_LEFT 
                    || this->buffer[this->currPositionInBuffer].getState () == STATE_OUTPUT_RESOLVED)
        {
			if(prevCellAvailable && this->buffer[this->currPositionInBuffer-1].getState() == STATE_RESOLVED_LEFT) //update previous cell
			{	
				this->buffer[this->currPositionInBuffer-1].setState (STATE_RESOLVED_RIGHT_LEFT);				
			}
		}
	}  
    //depending on the state of the resultingcell - update count in output state counters
    size_t totalInOutput = this->stateCounters[this->currentOutputSuffixID].size();
    int currentCellState = this->buffer[this->currPositionInBuffer].getState() ;
    if (totalInOutput == 0)
    {
        if (currentCellState== STATE_OUTPUT_RESOLVED)
            this->stateCounters[this->currentOutputSuffixID].push_back (-1);
        else
            this->stateCounters[this->currentOutputSuffixID].push_back(1);
    }
    else //look at the last element
    {
        int64 lastVal = this->stateCounters[this->currentOutputSuffixID][totalInOutput-1];
        if (lastVal < 0 ) //last value in vector - resolved interval
        {
            if (currentCellState== STATE_OUTPUT_RESOLVED) //increment count of resolved cells
                this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]--;
            else //start new unresolved interval
                this->stateCounters[this->currentOutputSuffixID].push_back(1);
        }
        else //last value in vector - unresolved interval
        {
            if (currentCellState== STATE_OUTPUT_RESOLVED) //start new resolved interval
               this->stateCounters[this->currentOutputSuffixID].push_back(-1); 
            else //increment count of unresolved cells
               this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]++;                 
        }
    }    
   
    *resultingCell=this->buffer[this->currPositionInBuffer];

	this->currPositionInBuffer++;
    this->currentElementInBin++;

    this->inCurrentURpos++;

//check if this is the time to switch state from unresolved to resolved
    if (this->inCurrentURpos == this->inIntervalURtotal) //next interval should be of resolved - negative value
    {
        if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end()) //no more values in the input vector
        {
            return RES_SUCCESS;
        }
        int64 val = *(this->currStateCounterIt);
        if (val > 0 ) {  //unresolved interval again - error
            displayWarning("OrderStateReadWriteBuffer - next interval is resolved - after resolved.");
		    return RES_FAILURE;
        }
        else {  //resolved interval
            this->inIntervalRtotal= - val; 
            this->inCurrentRpos=0;  

            this->inIntervalURtotal=0; 
            this->inCurrentURpos=0; 
        } 
        this->currStateCounterIt ++;     
    }

	return RES_SUCCESS;
}



int OrderStateReadWriteBuffer::flushCopyBuffer()
{
	//we will copy remaining of the input file from the previous iteration into the current iteration, 
    // in case they are needed for input on the next iterations
	if(this->currPositionInBuffer==-1) //buffer was never initialized yet
	{
		bool done_reading = false;
        while ( ! done_reading )
        {
            //fill in buffer from the corresponding file			
		    size_t read = fread(&this->buffer[0], sizeof(OrderCell),(size_t)(this->capacity),this->inputFile);
			if(read>0) //successful read - copy content of the buffer to the output file
			{
				this->totalElementsInBuffer=read;

                size_t written =  fwrite(&this->buffer[0],sizeof(OrderCell),read,this->outputFile);
			    if(written != read)
			    {
				    displayWarning("OrderStateReadWriteBuffer appending of output failed: not all "+T_to_string<size_t>(read) +" elements were written to output file "+this->outputFileName);
				    return RES_FAILURE;
			    }

                if (read < this->capacity)
                    done_reading = true;
			}
			else
			{
				done_reading = true;
			}			
		}
		return RES_PROCESSED;
	}

	//if this is a normal file - first check that there are elements in buffer and flush them to disk
	if(this->totalElementsInBuffer > 0) 
	{
		//write buffer to a current output file		
		size_t written =  fwrite(&this->buffer[0],sizeof(OrderCell),(size_t)(this->totalElementsInBuffer),this->outputFile);
		if(written != (size_t)this->totalElementsInBuffer)
		{
			displayWarning("OrderStateReadWriteBuffer writing of output failed: not all "+T_to_string<uint64>(this->totalElementsInBuffer) +" elements were written to output file "+this->outputFileName);
			return RES_FAILURE;
		}				
	}

	//now continue - copying the rest of the input file
	bool done_reading = false;
    while ( ! done_reading )
    {
        //fill in buffer from the corresponding file			
		size_t read = fread(&this->buffer[0], sizeof(OrderCell),(size_t)(this->capacity),this->inputFile);
		if(read>0) //successful read - copy content of the buffer to the output file
		{
			this->totalElementsInBuffer=read;

            size_t written =  fwrite(&this->buffer[0],sizeof(OrderCell),read,this->outputFile);
			if(written != read)
			{
				displayWarning("OrderStateReadWriteBuffer appending of output failed: not all "+T_to_string<size_t>(read) +" elements were written to output file "+this->outputFileName);
				return RES_FAILURE;
			}

            if (read < this->capacity)
                done_reading = true;
		}
		else
		{
			done_reading = true;
		}
    }

	return RES_PROCESSED;
}

//this refills upto capacity elements into a buffer, in the first slot there is the last element of the previous buffer 
int OrderStateReadWriteBuffer::flushRefillBuffer()
{
	if(this->currPositionInBuffer==-1) //buffer is not initialized yet
	{
		size_t read = fread(&this->buffer[0], sizeof(OrderCell),(size_t)(this->capacity),this->inputFile);
		if(read>0) //successful read 		
        {
			this->totalElementsInBuffer=read;
			this->currPositionInBuffer=0;
		}
		else
		{
			displayWarning("OrderStateReadWriteBuffer - first-time refill failed: File "+this->inputFileName+" is probably empty.");
			return RES_FAILURE;
		}
		
		return RES_SUCCESS;
	}

	//flush what is in buffer - if there is something, move last element in current buffer to the beginning of the buffer
	if (this->totalElementsInBuffer > 0) 
	{
		//if there is only 1 element - it means this is the last element of the bin, which was positioned here from the previous buffer
        if (this->totalElementsInBuffer == 1)
        {
            //if ( this->buffer [ 0].getState () == STATE_RESOLVED_LEFT)
            //    this->buffer [ 0].setState (STATE_RESOLVED_RIGHT_LEFT);

            //flush it to the output file
            if( fwrite(&this->buffer[0],sizeof(OrderCell),1,this->outputFile) != 1)
		    {
			    displayWarning("OrderStateReadWriteBuffer appending the last element of the bin to output file "+this->outputFileName + " falied.");
			    return RES_FAILURE;
		    }

            //nothing to be done
            return RES_PROCESSED;
        }        
        
        //always write minus 1 element
        uint64 totalToWrite=this->totalElementsInBuffer - 1 ;		

		//write buffer to a current file		
		size_t written =  fwrite(&this->buffer[0],sizeof(OrderCell),(size_t)(totalToWrite),this->outputFile);
		if(written != totalToWrite)
		{
			displayWarning("OrderStateReadWriteBuffer appending of output failed: not all "+T_to_string<uint64>(totalToWrite) +" elements were written to output file "+this->outputFileName);
			return RES_FAILURE;
		}

        //copy last element to the zero pos of the buffer
        this->buffer [0] = this->buffer [this->totalElementsInBuffer - 1];
        this->totalElementsInBuffer = 1 ;
        this->currPositionInBuffer = 1;
	}
    else
    {
        this->totalElementsInBuffer = 0 ;
        this->currPositionInBuffer = 0;
    }

	//we are done with writing current buffer content

	//now refill buffer from the corresponding file
    uint64 totaltorread = this->capacity;
    uint64 posInBuffer = 0;
	if ( this->totalElementsInBuffer == 1)
    {
        totaltorread --;
        posInBuffer++;
    }

	size_t read = fread (&this->buffer[posInBuffer], sizeof(OrderCell),(size_t)(totaltorread),this->inputFile);
	if (read > 0)
	{
		this->totalElementsInBuffer += read;
		this->currPositionInBuffer = posInBuffer;

        //here update prev element state if needed
       // if(posInBuffer == 1)
      //  {
      //      if ( this->buffer [0].getState () == STATE_RESOLVED_LEFT && this->buffer [1].getState () >= STATE_RESOLVED_LEFT)
      //          this->buffer [0].setState(STATE_RESOLVED_RIGHT_LEFT);
       // }
	}
	else
	{
		return RES_PROCESSED;
	}

	return RES_SUCCESS;
}


