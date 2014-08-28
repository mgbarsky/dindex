#include "OrderStateInputBuffer.h"

using namespace std;

OrderStateInputBuffer::OrderStateInputBuffer() 
{
	this->buffer=NULL;	
    this->file = NULL;
}
	
OrderStateInputBuffer::~OrderStateInputBuffer()
{
	delete [] this->buffer;
}

//memory allocation - occurs once per program
bool OrderStateInputBuffer::init( uint64 maxCapacity, short totalBins,
		const std::string &dbDir, const std::string &tempDir)
{
	this->capacity = maxCapacity;
    this->dbDir = dbDir;
	this->tempDir=tempDir;

    
	//allocate buffer
	this->buffer = new OrderCell[(size_t)this->capacity];

    if(!this->buffer)
    {
        displayWarning("OrderStateInputBuffer: buffer memory allocation for "+T_to_string<int64>(this->capacity)+" elements failed");
        return false;
    }

	this->totalBins = totalBins;
	
	this->totalElementsInBuffer=0;
	this->currPositionInBuffer=-1;

	this->currentInputSuffixID=1; //will change to 0 in reset
	return true;
}

bool OrderStateInputBuffer::reset()
{
	this->totalElementsInBuffer = 0;
	this->currPositionInBuffer = -1;

	this->currentBinID = 0;
	
	if(this->currentInputSuffixID == 1)
	{
		this->currentInputSuffixID=0;
		this->inputSuffix=".0";
	}
	else
	{
		this->currentInputSuffixID=1;
		this->inputSuffix=".1";
	}

    this->file = NULL;
    return true;
}

//read next buffer value into resultCell
int OrderStateInputBuffer::getNextOrderCell(OrderCell *resultCell)
{	
	if(this->currPositionInBuffer == -1)
	{
		int ret = this->refillBuffer();
		if (ret != RES_SUCCESS)
			return ret;
	}

	if(this->currPositionInBuffer == this->totalElementsInBuffer) //finished reading buffer cells
	{
		int ret = this->refillBuffer();
		if(ret != RES_SUCCESS)
			return ret;
	}

	*resultCell= this->buffer[this->currPositionInBuffer++];
	
	return RES_SUCCESS;
}

//private:
int OrderStateInputBuffer::refillBuffer() //refills and resets buffer position
{
	if(this->currentBinID == this->totalBins)  //all bins have been processed
		return RES_PROCESSED;

    if ( ! this->file ) //open next bin file if not already opened
    {
	    //construct input file name
	    this->inputFileName = this->tempDir +separator()+ "orderarray"
		    + T_to_string<short>(this->currentBinID)+this->inputSuffix;

        //open file	
	    this->file=fopen((this->inputFileName).c_str(),"rb");
	    if( ! this->file )
	    {
		    displayWarning ("OrderStateInputBuffer - refill failed: File "+this->inputFileName+" not found.");
		    return RES_FAILURE;
	    }	
    }
    
    size_t read = fread ( this->buffer, sizeof(OrderCell), (size_t) this->capacity, this->file);

	if(read>0)
	{
		this->totalElementsInBuffer=read;
		this->currPositionInBuffer=0;
	
		//finished this file, prepare for the next
		if(read < this->capacity)
        {
            this->currentBinID ++;
            fclose (this->file);
            this->file = NULL;
        }
	}
	else  //no more data in this file
	{
		this->currentBinID ++;
        fclose (this->file);
        this->file = NULL;

        return this->refillBuffer();
	}
   
	return RES_SUCCESS;
}


