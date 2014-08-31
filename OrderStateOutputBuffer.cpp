#include "OrderStateOutputBuffer.h"

using namespace std;

OrderStateOutputBuffer::OrderStateOutputBuffer() 
{
	this->buffer=NULL;	
	this->file=NULL;
}

OrderStateOutputBuffer::~OrderStateOutputBuffer()
{
	delete [] this->buffer;
}

bool OrderStateOutputBuffer::init(uint64 maxMemBytes, short totalBins, const std::string &tempDir)
{
	this->capacity=maxMemBytes/ sizeof(OrderCell);

	this->totalBins=totalBins;

	this->tempDir = tempDir;	

	//allocate buffer
	this->buffer =  new OrderCell[(size_t)this->capacity];
	
    if (!this->buffer)
        return false;
  	return true;
}

bool OrderStateOutputBuffer::initBin (short binID)
{
	this->currentBinID=binID;
    this->firstInBin = true;
	
    this->fileName = this->tempDir +separator()+ "orderarray"
		+ T_to_string<short>(this->currentBinID)+".0"; //suffix is .0 - because this buffer is only used during initialization

    this->file = fopen (this->fileName.c_str(),"wb");
    if ( !this->file) {
        displayWarning("OrderStateOutputBuffer initBin failed: cannot write to output file "+this->fileName);
		return false;
    }

	this->currPositionInBuffer=0;
	
   // printf("Started new bin %d in output order array during initialization. Buffer capacity= %ld\n",this->currentBinID,(long)this->capacity);
	return true;
}

bool OrderStateOutputBuffer::wrapUpCurrentBin()
{
    if (this->currPositionInBuffer > 0)
    {
        if ( !this->flush () )
            return false;
    }

    fclose (this->file);
    this->file = NULL;

    return true;
}

bool OrderStateOutputBuffer::initNextCell(short chunkID)
{
	if(this->currPositionInBuffer == this->capacity)
	{
		if(!this->flush())
			return false;
	}

	this->buffer[this->currPositionInBuffer].setChunkID ( chunkID );
	
	this->buffer[this->currPositionInBuffer].setResolvingChar ((char)this->currentBinID);
	
	this->buffer[this->currPositionInBuffer].setSplitCharsBeforeAfter(0,0); //undefined
	
	
	if(this->firstInBin)
	{
		this->buffer[this->currPositionInBuffer].setLCP (0); //first element in this bin		
		this->buffer[this->currPositionInBuffer].setSplitCharsBeforeAfter(0,(char)this->currentBinID);
		if (this->currentBinID == 0) //sentinel bin
			this->buffer[this->currPositionInBuffer].setState (STATE_RESOLVED_RIGHT_LEFT);
		else
			this->buffer[this->currPositionInBuffer].setState (STATE_RESOLVED_LEFT);

        this->firstInBin = false;
	}
	else
	{
		if(this->currentBinID == 0) //sentinel bin
		{
			this->buffer[this->currPositionInBuffer].setLCP (0); //dont care about lcps in sentinel bin
			this->buffer[this->currPositionInBuffer].setState (STATE_RESOLVED_RIGHT_LEFT);
		}
		else
		{
			this->buffer[this->currPositionInBuffer].setLCP (1); //share at least 1 common char	- belong to the same bin		
			this->buffer[this->currPositionInBuffer].setState (STATE_UNRESOLVED);
		}
	}
	
	this->currPositionInBuffer++;
	
	return true;
}

bool OrderStateOutputBuffer::flush()
{
	if(this->currPositionInBuffer == 0)
		return true;
	
	if ( fwrite (&this->buffer[0],sizeof(OrderCell),(size_t)this->currPositionInBuffer,this->file) 
        !=  (size_t)this->currPositionInBuffer)
	{
		displayWarning("OrderStateOutputBuffer flush of " +T_to_string<uint64>(this->currPositionInBuffer) +  " order cells failed: not all data has been written");
		return false;
	}

	this->currPositionInBuffer=0;
	
	return true;
}

