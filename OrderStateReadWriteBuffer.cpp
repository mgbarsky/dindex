#include "OrderStateReadWriteBuffer.h"

using namespace std;
void printStateVector ( std::vector<int64> &vec)
{
	for (int64 i=0; i< vec.size(); i++)
		printf (" %ld ", (long)vec[i]);
	printf("\n");
}
//constructor
OrderStateReadWriteBuffer::OrderStateReadWriteBuffer() 
{
	this->buffer = NULL;	
    this->inputFile = NULL;
    this->outputFile = NULL;
    this->finalOutputFile = NULL;
    this->finalHeaderFile = NULL;
    this->debug = false;
}

//destructor - free buffer memory	
OrderStateReadWriteBuffer::~OrderStateReadWriteBuffer()
{
	if (this->buffer != NULL) delete [] this->buffer;
}

//memory allocation - occurs once per program - in the beginning
bool OrderStateReadWriteBuffer::init( uint64 totalMemInBytes, unsigned char binID, 
		  const std::string &tempDir, int64 totalElementsInBin)
{
	this->capacity = totalMemInBytes/sizeof(OrderCell); 
	this->binID = binID;	
   
    //initialize buffer array
   	this->buffer = new OrderCell[(size_t)this->capacity];
    if(!this->buffer)
    {
        displayWarning("OrderStateReadWriteBuffer: buffer memory allocation for "+T_to_string<int64>(this->capacity)+" elements failed");
        return false;
    }
    this->totalElementsInBuffer=0;
	this->currPositionInBuffer=-1; //not initialized yet

    this->totalElementsInBin =totalElementsInBin;
    this->currentElementInBin =0;

    this->fileName0 = tempDir +separator()+ "orderarray"+ T_to_string<short>(this->binID)+".0";
    this->fileName1 = tempDir +separator()+ "orderarray"+ T_to_string<short>(this->binID)+".1";
    
    //initialize input state counter
    this->stateCounters[0].push_back (totalElementsInBin);
    this->currentInputSuffixID=1; //will switch once reset is called

    //open 2 file pointer to write final output for this bin - this will stay open for the entire program
    string finalFileName = tempDir +separator()+ "orderarray"+ T_to_string<short>(this->binID)+".final";
    this->finalOutputFile = fopen (finalFileName.c_str(),"wb");	
    if(!this->finalOutputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - init failed: cannot write to file "+finalFileName+".");
		return false;
	}

    string finalHeaderFileName = tempDir +separator()+ "orderarray"+ T_to_string<short>(this->binID)+".header";
	this->finalHeaderFile = fopen (finalHeaderFileName.c_str(),"wb");	
	if(!this->finalHeaderFile)
	{
		displayWarning("OrderStateReadWriteBuffer - init failed: cannot write to file "+finalHeaderFileName+".");
		return false;
	}	
	return true;
}

//this is called once per program - to close pointers to final files at the end
void OrderStateReadWriteBuffer::wrapUp()  
{
    fclose (this->finalOutputFile);
    this->finalOutputFile = NULL;
    
    fclose (this->finalHeaderFile);
    this->finalHeaderFile = NULL;
}

bool OrderStateReadWriteBuffer::nextIterationReset() //reads from file 0, writes updated state to file 1
{
    //set pointer to the input file
    this->inputFile = fopen (this->fileName0.c_str(),"rb");	

	if(!this->inputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - nextIterationReset failed: File "+this->fileName0+" cannot be found.");
		return false;
	} 

    //set pointer to the output file
    this->outputFile = fopen (this->fileName1.c_str(),"wb");	

	if(!this->outputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - nextIterationReset failed: cannot write to File "+this->fileName1+".");
		return false;
	} 

    //switch state vectors - they are pointing interchangeably to input and to output
    if(this->currentInputSuffixID == 1)
	{
		this->currentInputSuffixID=0;
        this->currentOutputSuffixID = 1;		
        this->stateCounters[1].clear(); //prepare output vector 1 for the next iteration

	}
	else
	{
		this->currentInputSuffixID=1;
        this->currentOutputSuffixID = 0;	
        this->stateCounters[0].clear(); //prepare output vector 0 for the next iteration
	}

    //reset buffer to the beginning
    this->currPositionInBuffer=-1;
	this->totalElementsInBuffer=0;


    //reset total bin counter to the beginning
    this->currentElementInBin =0;

    //reset all interval variables to zero
    this->inIntervalRtotal=0; 
    this->inCurrentRpos=0;  

    this->inIntervalURtotal=0; 
    this->inCurrentURpos=0; 
    
    //set iterator to the beginning of the input states vector
    this->currStateCounterIt = this->stateCounters[currentInputSuffixID].begin();
    this->currentPosInFinalOutput = 0;

    return true;
}

bool OrderStateReadWriteBuffer::wrapUpIteration ()
{
	printf ("Input in buffer %d:\n", this->binID);
    printStateVector (this->stateCounters[this->currentInputSuffixID]);
	printf ("Outputin buffer %d:\n", this->binID);
	printStateVector (this->stateCounters[this->currentOutputSuffixID]);
    
    if (this->totalElementsInBin == 0)
	{
		if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
			return true;
		int64 lastInVal = *this->currStateCounterIt;
		this->stateCounters[this->currentOutputSuffixID].push_back(lastInVal);
		return true;
	}
	if (!this->flush())
        return false;   

    fclose (this->inputFile);
    this->inputFile = NULL;

    fclose (this->outputFile);
    this->outputFile = NULL;
    
    //copy the remaining entry from input state to output state - the only one which may happen is resolved tail
    if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
         return true;
    int64 lastInVal = *this->currStateCounterIt;

    if (lastInVal >0)
    {
        displayWarning("OrderStateReadWriteBuffer wrapUpIteration failed - unprocessed values remain in input state vector.");
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


    return true;
}


bool OrderStateReadWriteBuffer::nextBifurcationReset() //reads from file 1, writes updated state to file 0
{
    //set pointer to the input file
    this->inputFile = fopen (this->fileName1.c_str(),"rb");	

	if(!this->inputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - nextIterationReset failed: File "+this->fileName1+" cannot be found.");
		return false;
	} 

    //set pointer to the output file
    this->outputFile = fopen (this->fileName0.c_str(),"wb");	

	if(!this->inputFile)
	{
		displayWarning("OrderStateReadWriteBuffer - nextIterationReset failed: cannot write to File "+this->fileName0+".");
		return false;
	} 
   
    //reset buffer to the beginning
    this->currPositionInBuffer=-1;
	this->totalElementsInBuffer=0;

    //reset total bin counter to the beginning
    this->currentElementInBin =0;

    //reset all interval variables to zero
    this->inIntervalRtotal=0; 
    this->inCurrentRpos=0;  

    this->inIntervalURtotal=0; 
    this->inCurrentURpos=0; 
    
    //set iterator to the beginning of the old input states vector - to keep track of current position in bin
    this->currStateCounterIt = this->stateCounters[currentInputSuffixID].begin();

    return true;
}

bool OrderStateReadWriteBuffer::wrapUpBifurcation ()
{
    if (!this->flushBifurcation())
        return false;   

    fclose (this->inputFile);
    this->inputFile = NULL;

    fclose (this->outputFile);
    this->outputFile = NULL;    
    
    return true;
}

int OrderStateReadWriteBuffer::getNextCell (OrderCell *resultingCell)
{
    int ret = RES_SUCCESS;

    if (this->currPositionInBuffer == -1) 
    {
        ret = this->refill();
        if (ret !=RES_SUCCESS)
            return ret;
        //if success - then current position in buffer is on 0
    }

    //check that we are not at the end of the buffer
    if (this->currPositionInBuffer == this->totalElementsInBuffer)
    {
        if (!this->flushBifurcation())
            return RES_FAILURE;
        ret = this->refill();
        if (ret !=RES_SUCCESS)
            return ret;
        //if success - then current position in buffer is on 0
    }
	*resultingCell = this->buffer[this->currPositionInBuffer];
    this->currPositionInBuffer ++;    
    
    return ret;
}

int OrderStateReadWriteBuffer::refill()  //refills next chunk of the input file
{
    size_t read = fread(this->buffer, sizeof(OrderCell),(size_t)(this->capacity),this->inputFile);
	if(read>0) //successful read 		
    {
		this->totalElementsInBuffer=read;
		this->currPositionInBuffer=0;
	}
	else
	{		
		return RES_PROCESSED;
	}
	
	return RES_SUCCESS;
}

//here it writes to outputFile only values which are not resolved after latest iteration
bool OrderStateReadWriteBuffer::flushBifurcation() 
{
	if (this->totalElementsInBuffer == 0)
        return true;   

    int64 i=0;
    for (;i<this->totalElementsInBuffer;i++)
    {
        if (this->buffer[i].getState() != STATE_OUTPUT_RESOLVED) //write to output file, to serve as prev value for the next iteration
        {           
            //write entry to the output file
            if( fwrite(&this->buffer[i],sizeof(OrderCell),1,this->outputFile) != 1)
            {
                displayWarning("OrderStateReadWriteBuffer - flushBifurcation failed - cannot write next value to the output file.");
                return false;
            }
        }
    }
	this->currPositionInBuffer = -1;
	this->totalElementsInBuffer = 0;
    return true;       
}

//this updates current cell in buffer using the information in the same cell from previous iteration and resolving oprder cell
//it returns the result: 0 - failed, 1-success, 2-no more cells to update
//if the state of the current cell gets resolved at the end, it will also add this cell to the final output
int OrderStateReadWriteBuffer::updateNextOrderCell (InOrderCell *resolvingCell, char *finalState ) 
{
    int ret = RES_SUCCESS;
	if(this->currPositionInBuffer ==-1) {  //read first value from input state buffer
        //check that not after end 
        if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
        {
            displayWarning("OrderStateReadWriteBuffer - invalid iterator position in state interval vector.");
		    return RES_FAILURE;
        }

        int64 val = *(this->currStateCounterIt);
        if (val > 0 ) {  //unresolved interval
            //this->inIntervalRtotal=0; 
            //this->inCurrentRpos=0;  

            this->inIntervalURtotal=val; 
            this->inCurrentURpos=0; 
            this->currStateCounterIt ++;
        }
        else {  //resolved interval 
            // we need to switch to an unresolved interval in this case - 
            // because our next cell in buffer is from this interval
            
            //advance overall counter
            this->currentPosInFinalOutput +=(-val);
            
            //copy this initial value to the output vector state
            //record total resolved to the last position of the output state array
            this->stateCounters[this->currentOutputSuffixID].push_back(val); 
            
            //advance to the next entry of input state vector
            this->currStateCounterIt ++;
            //read next value
            if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
            {                
		        return RES_PROCESSED; //all values have been processed
            }

            int64 val = *(this->currStateCounterIt);
            if (val < 0) //error - because we cannot have unresolved after unresolved
            {
                 displayWarning("OrderStateReadWriteBuffer - during next cell update resolved interval comes after resolved.");
		         return RES_FAILURE;
            }

            //this->inIntervalRtotal=0; 
            //this->inCurrentRpos=0;  

            this->inIntervalURtotal=val; 
            this->inCurrentURpos=0; 
            this->currStateCounterIt ++;             
        } 
        //now staying on current interval - unresolved 
        //fill buffer for the first time
        ret = refill();  
        if (ret != RES_SUCCESS)
            return ret;  
        //this is now pointing to the first element in buffer  
    }
    
    //check if this is the time to switch state from unresolved to resolved and back
    if (this->inCurrentURpos == this->inIntervalURtotal) //next interval should be of resolved - negative value
    {        
        if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end()) //no more values in the input vector
        {
            return RES_PROCESSED;
        }
        int64 val = *(this->currStateCounterIt);
        if (val > 0 ) {  //unresolved interval again - error
            displayWarning("OrderStateReadWriteBuffer - next interval is unresolved - after unresolved.");
		    return RES_FAILURE;
        }
        else {  //resolved interval, switch it to the next unresolved
            
            //advance overall counter
            this->currentPosInFinalOutput += (-val);
            
            //add this value to the last entry of the output vector state
            size_t totalInOutput = this->stateCounters[this->currentOutputSuffixID].size();    
            if (totalInOutput == 0)
            {               
                this->stateCounters[this->currentOutputSuffixID].push_back (val);
            }
            else //look at the last element
            {
                int64 lastVal = this->stateCounters[this->currentOutputSuffixID][totalInOutput-1];
                if (lastVal < 0 ) //last value in vector - resolved interval
                {
                    this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]+=val;
                }
                else //last value in vector - unresolved interval
                {
                    //start new resolved interval
                    this->stateCounters[this->currentOutputSuffixID].push_back(val);                            
                }
            }    
            
            //advance to the next entry of input state vector
            this->currStateCounterIt ++;
            //read next value
            if (this->currStateCounterIt == this->stateCounters[currentInputSuffixID].end())
            {                
		        return RES_PROCESSED; //all values have been processed
            }

            int64 val = *(this->currStateCounterIt);
            if (val < 0) //error - because we cannot have resolved after resolved
            {
                 displayWarning("OrderStateReadWriteBuffer - during next cell update resolved interval comes after resolved.");
		         return RES_FAILURE;
            }

            //this->inIntervalRtotal=0; 
            //this->inCurrentRpos=0;  
            
            this->inIntervalURtotal=val; 
            this->inCurrentURpos=0; 
            //this->currStateCounterIt ++;  
        } 
         this->currStateCounterIt ++;    
    }

    if(	this->currPositionInBuffer == this->totalElementsInBuffer)  //need to flush data and to refill new data
	{		
        int ret = this->flushRefill();
		if (ret != RES_SUCCESS)
			return ret;
        //this always points to position 1, because last element of previous buffer is copied to the entry 0
	}

    //default state - the cell is in the middle of the buffer array
    bool prevCellAvailable = true;
    bool nextCellAvailable = true;
    bool lastInBin = false;
    bool nextCellResolved = false;
    //bool prevCellResolved = false;
    
    if (this->currPositionInBuffer == 0) //there is no previous cell in buffer to look at
        prevCellAvailable = false;

    if  (this->currentElementInBin == this->totalElementsInBin - 1) //last cell in the entire bin
        lastInBin = true;

    //now about the next cell
    if (this->currPositionInBuffer == this->totalElementsInBuffer - 1) //last cell of this buffer
        nextCellAvailable = false;   

    if (this->inCurrentURpos == 0) //first unresolved cell in this interval
    {
        //prevCellResolved =true;
        prevCellAvailable = false;
    }

    if (this->inCurrentURpos == this->inIntervalURtotal - 1) //last cell of current unresolved interval
    {
        nextCellResolved = true;
        nextCellAvailable = false;
    }

    //we are staying on the next unresolved cell
    if(this->buffer[this->currPositionInBuffer].getState () == STATE_OUTPUT_RESOLVED) 
    {
        displayWarning("OrderStateReadWriteBuffer - found resolved order cell where unresolved expected.");
		return RES_FAILURE;
    }

    //we now are staying on the current cell which is not yet fully STATE_OUTPUT_RESOLVED
    
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
    *finalState = this->buffer[this->currPositionInBuffer].getState() ;
    if (totalInOutput == 0)
    {
        if (*finalState== STATE_OUTPUT_RESOLVED)
            this->stateCounters[this->currentOutputSuffixID].push_back (-1);
        else
            this->stateCounters[this->currentOutputSuffixID].push_back(1);
    }
    else //look at the last element
    {
        int64 lastVal = this->stateCounters[this->currentOutputSuffixID][totalInOutput-1];
        if (lastVal < 0 ) //last value in vector - resolved interval
        {
            if (*finalState== STATE_OUTPUT_RESOLVED) //increment count of resolved cells
                this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]--;
            else //start new unresolved interval
                this->stateCounters[this->currentOutputSuffixID].push_back(1);
        }
        else //last value in vector - unresolved interval
        {
            if (*finalState== STATE_OUTPUT_RESOLVED) //start new resolved interval
               this->stateCounters[this->currentOutputSuffixID].push_back(-1); 
            else //increment count of unresolved cells
               this->stateCounters[this->currentOutputSuffixID][totalInOutput-1]++;                 
        }
    }    
    
    // also if state is now resolved - copy this cell to final output: TBD - write correct header when state switches
    if (*finalState == STATE_OUTPUT_RESOLVED)
    {
        if( fwrite(&this->buffer[this->currPositionInBuffer],sizeof(OrderCell),1,this->finalOutputFile) != 1)
        {
            displayWarning("OrderStateReadWriteBuffer - update next cell failed - cannot write next resolved value to the final output file.");
            return RES_FAILURE;
        }
    }    

	this->currPositionInBuffer++;
    this->currentElementInBin++;

    this->inCurrentURpos++;
    this->currentPosInFinalOutput ++;

	return RES_SUCCESS;
}

bool OrderStateReadWriteBuffer::flush() //flushes content of a buffer - full content - after iteration
{
    //flush what is in buffer - if there is something
	if (this->totalElementsInBuffer > 0) 
	{ 
        size_t written =  fwrite(&this->buffer[0],sizeof(OrderCell),(size_t)(this->totalElementsInBuffer),this->outputFile);
		if(written != (size_t)this->totalElementsInBuffer)
		{
			displayWarning("OrderStateReadWriteBuffer flush - appending of output failed: not all "+T_to_string<uint64>(this->totalElementsInBuffer) +" elements were written to output file.");
			return false;
		}
    }
    return true;
}

//this refills upto capacity elements into a buffer, in the first slot there is the last element of the previous buffer 
int OrderStateReadWriteBuffer::flushRefill()
{
	if(this->currPositionInBuffer==-1) //buffer is not initialized yet
	{
		return this->refill();
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
			    displayWarning("OrderStateReadWriteBuffer appending the last element of the bin to output file during iteration falied.");
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
			displayWarning("OrderStateReadWriteBuffer appending of output failed: not all "+T_to_string<uint64>(totalToWrite) +" elements were written to output file during iteration.");
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
	}
	else
	{
		return RES_PROCESSED;
	}

	return RES_SUCCESS;
}


