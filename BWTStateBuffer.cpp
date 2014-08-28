#include "BWTStateBuffer.h"

using namespace std;

//public:
//constructor - set default values
BWTStateBuffer::BWTStateBuffer()
{
    this->buffer = NULL; 
    this->lfTable = NULL;
    this->inputFile = NULL;
	this->outputFile = NULL;
}

//destructor
BWTStateBuffer::~BWTStateBuffer()
{
    if (this->buffer != NULL) delete [] this->buffer;
    if (this->lfTable != NULL) delete [] this->lfTable;
}

//initializes memory 
//counts total different characters in BWT file
//writes temp copy into temp dir
bool BWTStateBuffer::init( uint64 maxBufferCapacityBytes, InputReader *reader,
                short chunkID, std::string const &dbDir, std::string const &tempDir,  std::string const &bwtFileExt)
{
	//initialize values
    this->chunkID=chunkID;
    string originalFileName = dbDir	 + separator() + T_to_string<short>(this->chunkID)+bwtFileExt;

    //set file names
    this->fileName0 = tempDir + separator() + T_to_string<short>(this->chunkID)+bwtFileExt +".0"; 
    this->fileName1 = tempDir + separator() + T_to_string<short>(this->chunkID)+bwtFileExt +".1"; 

	this->totalLetterBins = reader->getAlphabetSize()+1;

	//allocate buffer to read the file 
	this->capacity = maxBufferCapacityBytes/sizeof(char);
	this->buffer = new char[(size_t)this->capacity];

	//allocate LF table
	this->lfTable = new uint64[this->totalLetterBins];
	for(unsigned short i=0; i<this->totalLetterBins; i++)
		this->lfTable[i]=0;	

	//read entire chunk file into buffer and count occurrences of each character
	//also write BWT file into temp dir, with suffix .0 to prepare for the first iteration		
	this->inputFile=fopen(originalFileName.c_str(),"rb");

	if( ! this->inputFile)	{
		displayWarning ("BWTStateBuffer init: File " + originalFileName + " not found.");
		return false;
	}

	this->outputFile=fopen((this->fileName0).c_str(),"wb");

	if( ! this->outputFile)	{
		displayWarning ("BWTStateBuffer init: Cannot write to file " + this->fileName0 + ".");
		return false;
	}
	
	bool done_reading = false;
	while(!done_reading)
	{
		//read piece of a file into ram
		size_t read = fread (this->buffer,sizeof(char),(size_t)this->capacity,this->inputFile);
		if(read > 0) //process 
		{
			for(size_t pos=0; pos< read; pos++)
			{
				unsigned short binID = reader->getBinID(this->buffer[pos]);
				this->lfTable[binID]++;
			}	

			//write into temp file for first iteration
			size_t written =  fwrite(this->buffer,sizeof(char),read,this->outputFile);
			if (written != read)
			{
				displayWarning("BWTStateBuffer init: error writing to file " + this->fileName0 + ": not all has been written.");
				return false;
			}

            //if read less than allowed - we finished the file
			if(read < this->capacity)
				done_reading = true;
		}
		else
		{
			done_reading = true;
		}		
	}
	fclose(this->inputFile);
	this->inputFile=NULL;

	fclose(this->outputFile);
	this->outputFile=NULL;
	
	//reset buffer to the beginning
    this->currPositionInFile=0;
	this->currPositionInBuffer=-1; //buffer is empty yet for our purposes
	this->totalElementsRead=0;
	
	return true;
}

uint64 BWTStateBuffer::getLetterBinContribution(unsigned short binID) {
	return this->lfTable[binID];
}

bool BWTStateBuffer::nextIterationReset()  //this resets all the variables to the beginning and switches the input to 0 and output to 1
{
    this->mode = ITERATION_STATE_UPDATE_MODE;
    
//set file 0 for input, file 1 for output
    this->inputFile=fopen((this->fileName0).c_str(),"rb");

	if( ! this->inputFile)	{
		displayWarning ("BWTStateBuffer nextIterationReset: File " +this->fileName0 + " not found.");
		return false;
	}

    fclose (this->inputFile);
    this->inputFile = NULL;

	this->outputFile=fopen(( this->fileName1).c_str(),"wb");

	if( ! this->outputFile)	{
		displayWarning ("BWTStateBuffer nextIterationReset: Cannot write to file " +  this->fileName1 + ".");
		return false;
	}

    fclose (this->outputFile);
    this->outputFile = NULL;

    //reset buffer to the beginning
    this->currPositionInFile = 0;
	this->currPositionInBuffer = -1; //empty buffer flag
	this->totalElementsRead = 0;
	
	return true;
}

bool BWTStateBuffer::nextBifurcationReset()  //this resets all the variables to the beginning and switches the input to 1 and output to 0
{
    this->mode = BIFURCATION_FILTER_MODE;

    //set file 1 for input, file 0 for output 
    this->inputFile=fopen((this->fileName1).c_str(),"rb");

	if( ! this->inputFile)	{
		displayWarning ("BWTStateBuffer nextBifurcationReset: File " + this->fileName1 + " not found.");
		return false;
	}

    fclose (this->inputFile);
    this->inputFile = NULL;

	this->outputFile=fopen(( this->fileName0).c_str(),"wb");

	if( ! this->outputFile)	{
		displayWarning ("BWTStateBuffer nextBifurcationReset: Cannot write to file " +  this->fileName0 + ".");
		return false;
	}

    fclose (this->outputFile);
    this->outputFile = NULL;

    //reset buffer to the beginning
    this->currPositionInFile = 0;
	this->currPositionInBuffer = -1; //empty buffer flag
	this->totalElementsRead = 0;
	
	return true;
}

//advances to the next BWT element of the input during iteration - the buffer is always read from somewhere
int BWTStateBuffer::next()
{
	if (this->currPositionInBuffer == -1 ) //first-time buffer refill
	{
		return (this->refill());
		//in case of success - pointing to the first element in the newly re-filled buffer
	}

	//advance position in current buffer
    this->currPositionInBuffer++;

    //check that this is not the last element of the buffer
    if (this->currPositionInBuffer == this->totalElementsRead) //we finished processing elements of this buffer - need refill
	{
		if (!this->flush ())
            return RES_FAILURE;
        int ret = this->refill();
        if (ret != RES_SUCCESS)
            return ret;	        
        //in case of success - pointing to the first element in the newly re-filled buffer
        //in case no more entries returns RES_PROCESSED
	}

	return RES_SUCCESS;
}

//during iteration we need to get current cell of chunk BWT in order to know which bin to resolve, and to later update its state
//need to call next() before accessing current cell
char BWTStateBuffer::getCurrentCell() {
	return (this->buffer[this->currPositionInBuffer]);
}

//during iteration we need to update state of the same cell - so we stay on it
void BWTStateBuffer::setCurrentCell(char newCell) { 	
    this->buffer[this->currPositionInBuffer] = newCell;
}

//during bifurcation we just look at the state of each next cell in some order
//and then only unresolved chars from the same buffer are written to an output file 
int BWTStateBuffer::getNextCell(char *resultCell) {
    int ret = RES_SUCCESS;

    if (this->currPositionInBuffer == -1 ) //first-time buffer refill
	{
        ret = this->refill();
        if (ret != RES_SUCCESS)
            return ret;		
		//in case of success - pointing to the first element in the newly re-filled buffer
	}
    

    if (this->currPositionInBuffer == this->totalElementsRead) //we finished processing elements of this buffer - need refill
	{
        if (!this->flushUnresolved ())
            return RES_FAILURE;
        ret = this->refill();
        if (ret != RES_SUCCESS)
            return ret;	
    }

    *resultCell = this->buffer[this->currPositionInBuffer];

    this->currPositionInBuffer++;

    return RES_SUCCESS;	
    
}

//private:
//this is called only first time after reset
int BWTStateBuffer::refill() {
    int res = RES_SUCCESS;

    //fill buffer for the corresponding chunk file
	this->inputFile=fopen((this->inputFileName).c_str(),"rb");
	if(!this->inputFile)	{
		displayWarning("BWTStateBuffer -refill failed: File "+this->inputFileName+" not found.");
		return RES_FAILURE;
	}	

    //move file pointer to the corresponding position within the file
    int ret=fseeklarge ( this->inputFile , this->currPositionInFile , SEEK_SET);
	if (ret!=0)    {
		res = RES_FAILURE;
    }
    else    {
	    size_t read = fread(this->buffer, sizeof(char), (size_t)this->capacity, this->inputFile);
	    if(read>0)	{
		    this->totalElementsRead = read;
		    this->currPositionInBuffer = 0;
            this->currPositionInFile += read; 
	    }
	    else {	//no more elements in file	    	   
		    res = RES_PROCESSED;
	    }
    }
	fclose (this->inputFile);
	this->inputFile = NULL;

	return res;
}

//this is called to flush what is in the buffer
///in iteration - we flush updated values as is - output is in file 1
bool BWTStateBuffer::flush() 
{    
    if (this->totalElementsRead == 0) //nothing to write
        return true;
	
    //open output file for appending
	this->outputFile=fopen((this->fileName1).c_str(),"ab"); //appends to the end of already existing file

	if ( !this->outputFile )		{
		displayWarning("BWTStateBuffer flush failed: cannot append to output file "+this->fileName1);
		return false;
	}	

    size_t written =  fwrite(this->buffer,sizeof(char),(size_t)(this->totalElementsRead),this->outputFile);
    if (written != (size_t)this->totalElementsRead)		{
	    displayWarning("BWTStateBuffer writing of output failed: not all "+T_to_string<uint64>( this->totalElementsRead) +" elements were not written to output file "+this->fileName1);
	    return false;
    }   

	fclose(this->outputFile);	
	this->outputFile = NULL;		
	
	return true;
}

//this is called during bifurcation - so the output is in file0
bool BWTStateBuffer::flushUnresolved () 
{
     if (this->totalElementsRead == 0) //nothing to write
        return true;
	
    //open output file for appending
	this->outputFile=fopen((this->fileName0).c_str(),"ab"); //appends to the end of already existing file

	if ( !this->outputFile )		{
		displayWarning("BWTStateBuffer flushUnresolved failed: cannot append to output file "+this->fileName0);
		return false;
	}	

    //write only chars which do not have first bit set - to indicate they are not needed for further processing
    int64 i=0;
    for (; i< this->totalElementsRead; i++)
    {
        if (this->buffer[i] > 0)
        {
            if ((fwrite(&this->buffer[i],sizeof(char),1,this->outputFile))!=1) {
                displayWarning("BWTStateBuffer flushUnresolved failed: could not write unresolved element to output file "+this->fileName0);
	            return false;
            }
        }
    }
   
	fclose(this->outputFile);	
	this->outputFile = NULL;		
	
	return true;
}

