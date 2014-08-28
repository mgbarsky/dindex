#include "OrderStateReadWriteBuffer.h"

using namespace std;

OrderStateReadWriteBuffer::OrderStateReadWriteBuffer() 
{
	this->buffer = NULL;	
    this->inputFile = NULL;
    this->outputFile = NULL;
}
	
OrderStateReadWriteBuffer::~OrderStateReadWriteBuffer()
{
	delete [] this->buffer;
}

//memory allocation - occurs once per program
bool OrderStateReadWriteBuffer::init( uint64 maxCapacity, unsigned char binID, 
		const std::string &dbDir,  const std::string &tempDir)
{
	this->capacity = maxCapacity; 
	this->binID = binID;
	
    this->dbDir = dbDir;
	this->tempDir=tempDir;

   	this->buffer = new OrderCell[(size_t)this->capacity];

    this->totalElementsInBin =0;
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
	if(this->currentInputSuffixID == 1)
	{
		this->currentInputSuffixID=0;
		this->inputSuffix=".0";
		this->outputSuffix=".1";
	}
	else
	{
		this->currentInputSuffixID=1;
		this->inputSuffix=".1";
		this->outputSuffix=".0";
	}

	this->currPositionInBuffer=-1;
	this->totalElementsInBuffer=0;
	
    this->currentElementInBin =0;
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

    return RES_PROCESSED;
}

//this updates current cell in buffer using the information in the same cell from previous iteration and resolving oprder cell
//it returns the result: 0 - failed, 1-success, 2-no more cells to update
int OrderStateReadWriteBuffer::updateNextOrderCell (OrderCell *resolvingCell, OrderCell *resultingCell) 
{
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
    bool nextCellResolved = this->buffer[this->currPositionInBuffer].isNextResolved();
    bool prevCellResolved = this->buffer[this->currPositionInBuffer].isPreviousResolved();
    if (prevCellResolved) prevCellAvailable=false;

    if (this->currPositionInBuffer == 0) //also there is no previous cell to look at
        prevCellAvailable = false;

    
	while(this->buffer[this->currPositionInBuffer].getState () == STATE_OUTPUT_RESOLVED) //continue increment counter - this copies prev value to the output buffer
	{
		this->currPositionInBuffer++;
        this->currentElementInBin++;
		if(	this->currPositionInBuffer == this->totalElementsInBuffer)  //need to flush data and to reset
		{	
			int ret = this->flushRefillBuffer();
			if (ret!=RES_SUCCESS)
				return ret;
		}       
	}

   
    //now about the next cell
    if (this->currPositionInBuffer == this->totalElementsInBuffer - 1) //last cell of this buffer
        nextCellAvailable = false;   
    
    if  (this->currentElementInBin == this->totalElementsInBin - 1) //last cell in the entire bin
        lastInBin = true;
    //we now are staying on the current cell which is not yet fully STATE_OUTPUT_RESOLVED
    
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
	
    //set state of the previous cell - if current cell is totally resolved - it would not be visible in the next iteration
    if (this->buffer[this->currPositionInBuffer].getState () == STATE_OUTPUT_RESOLVED && prevCellAvailable)
        this->buffer[this->currPositionInBuffer-1].setNextResolved(1);

    //set state of the current cell based on value of a previous cell - previous cell will not be seen in the next iteration
    if (prevCellAvailable && this->buffer[this->currPositionInBuffer-1].getState() == STATE_OUTPUT_RESOLVED)
        this->buffer[this->currPositionInBuffer].setPreviousResolved(1);

	*resultingCell=this->buffer[this->currPositionInBuffer];
	this->currPositionInBuffer++;
    this->currentElementInBin++;

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
        if(posInBuffer == 1)
        {
            if ( this->buffer [0].getState () == STATE_RESOLVED_LEFT && this->buffer [1].getState () >= STATE_RESOLVED_LEFT)
                this->buffer [0].setState(STATE_RESOLVED_RIGHT_LEFT);
        }
	}
	else
	{
		return RES_PROCESSED;
	}

	return RES_SUCCESS;
}


