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
    delete [] this->buffer;
    delete [] this->lfTable;
}

//initializes memory 
//counts total different characters in BWT file
//writes temp copy into temp dir
bool BWTStateBuffer::init( uint64 maxBufferCapacity, InputReader *reader,
                short chunkID, std::string const &dbDir, std::string const &tempDir,  std::string const &bwtFileExt)
{
	//initialize values	
	this->dbDir = dbDir;
	this->tempDir = tempDir;
	this->bwtFileExt=bwtFileExt;

	this->chunkID=chunkID;
	this->totalLetterBins = reader->getAlphabetSize()+1;

	//allocate buffer to read the file - align to 4 bytes
	this->capacity = maxBufferCapacity;
	this->buffer = new char[(size_t)this->capacity];

	//allocate LF table
	this->lfTable = new uint64[this->totalLetterBins];
	for(unsigned short i=0; i<this->totalLetterBins; i++)
		this->lfTable[i]=0;

	this->currentInputSuffixID=1;
	this->inputSuffix=".1";
	this->outputSuffix=".0";  //we are going to write BWT into temp dir, with suffix .0 to prepare for the first iteration

	//read entire chunk file into buffer and count occurrences of each character
	//also write BWT file into temp dir, with suffix .0 to prepare for the first iteration
	this->inputFileName=this->dbDir + separator() + T_to_string<short>(this->chunkID)+this->bwtFileExt;		
	this->inputFile=fopen((this->inputFileName).c_str(),"rb");

	if( ! this->inputFile)	{
		displayWarning ("BWTStateBuffer init: File " + this->inputFileName + " not found.");
		return false;
	}

	this->outputFileName=this->tempDir + separator() + T_to_string<short>(this->chunkID)+this->bwtFileExt+this->outputSuffix;		
	this->outputFile=fopen((this->outputFileName).c_str(),"wb");

	if( ! this->outputFile)	{
		displayWarning ("BWTStateBuffer init: Cannot write to file " + this->outputFileName + ".");
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

			//write into temp file
			size_t written =  fwrite(this->buffer,sizeof(char),read,this->outputFile);
			if (written != read)
			{
				displayWarning("BWTStateBuffer init: error writing to file " + this->outputFileName + ": not all has been written.");
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
	
	//set initial values	
    this->currPositionInFile=0;
	this->curPositionInBuffer=-1; //buffer is empty yet for our purposes
	this->totalElementsInBuffer=0;
	
	return true;
}

uint64 BWTStateBuffer::getLetterBinContribution(unsigned short binID)
{
	return this->lfTable[binID];
}

bool BWTStateBuffer::reset()  //this resets all the variables to the beginning and switches the input and output
{
    if(this->currentInputSuffixID == 0)
	{
		this->currentInputSuffixID=1;
		this->inputSuffix=".1";
		this->outputSuffix=".0";
	}
	else
	{
		this->currentInputSuffixID=0;		
		this->inputSuffix=".0";
		this->outputSuffix=".1";
	}

    //connstruct input and output file names
    this->inputFileName=this->tempDir + separator() + T_to_string<short>(this->chunkID)+this->bwtFileExt+this->inputSuffix;
    this->outputFileName=this->tempDir + separator() + T_to_string<short>(this->chunkID)+this->bwtFileExt+this->outputSuffix;
	
    //touch current output file to delete previous data
    this->outputFile=fopen(this->outputFileName.c_str(),"wb"); //clears previous file content

    if ( !this->outputFile )   {
        displayWarning("BWTStateBuffer reset: cannot write to file " + this->outputFileName + ".");
		return false;
    }

    fclose (this->outputFile);
    this->outputFile = NULL;

    //set initial values
    this->currPositionInFile = 0;
	this->curPositionInBuffer = -1; //empty buffer flag
	this->totalElementsInBuffer = 0;
	
	return true;
}

//advances to the next BWT element of the input
int BWTStateBuffer::next()
{
	if (this->curPositionInBuffer == -1) //first-time buffer refill
	{
		return this->refillReadOnly();
		//in case of success - pointing to the first element in the newly re-filled buffer
	}

	//advance position in current buffer
    this->curPositionInBuffer++;

    //check that this is the last element of the buffer
    if (this->curPositionInBuffer == this->totalElementsInBuffer) //we finished processing elements of this buffer - need refill
	{
		return (this->flushRefill());
        //in case of success - pointing to the first element in the newly re-filled buffer
        //in case no more entries returns RES_PROCESSED
	}

	return RES_SUCCESS;
}

void BWTStateBuffer::setCurrentElementProcessed()  //sets to 1 first bit of the corresponding BWT char
{
	if(this->buffer[this->curPositionInBuffer]<0)
        return;
    this->buffer[this->curPositionInBuffer] = -this->buffer[this->curPositionInBuffer];
}

char BWTStateBuffer::getCurrentElement()
{
	return (this->buffer[this->curPositionInBuffer]);
}

//private:
//this is called only first time after reset
int BWTStateBuffer::refillReadOnly()
{
    //fill buffer for the corresponding chunk file
	this->inputFile=fopen((this->inputFileName).c_str(),"rb");
	if(!this->inputFile)
	{
		displayWarning("BWTStateBuffer - first-time refill: File "+this->inputFileName+" not found.");
		return RES_FAILURE;
	}	

	size_t read = fread(this->buffer, sizeof(char),(size_t)this->capacity,this->inputFile);
	if(read>0)
	{
		this->totalElementsInBuffer = read;
		this->curPositionInBuffer = 0;
        this->currPositionInFile += read; 
	}
	else //file is empty
	{		
		displayWarning("BWTStateBuffer - first-time refill: File "+this->inputFileName+" is probably empty.");
		return RES_FAILURE;
	}
	fclose (this->inputFile);
	this->inputFile = NULL;

	return RES_SUCCESS;
}

//this is a public method called at the end to flush what remains in buffer
bool BWTStateBuffer::flushWriteOnly() //tbd - check maybe it is not needed
{
	if(this->totalElementsInBuffer > 0 )
	{
		//open file for appending
		this->outputFile=fopen(this->outputFileName.c_str(),"ab"); //appends to the end of already existing file

		if ( !this->outputFile )
		{
			displayWarning("BWTStateBuffer flushWriteOnly writing of output failed: cannot append to output file "+this->outputFileName);
			return false;
		}
	
		size_t written =  fwrite(this->buffer,sizeof(char),(size_t)(this->totalElementsInBuffer),this->outputFile);
		if (written != (size_t)this->totalElementsInBuffer)
		{
			displayWarning("BWTStateBuffer writing of output failed: not all "+T_to_string<uint64>( this->totalElementsInBuffer) +" elements were not written to output file "+this->outputFileName);
			return false;
		}
		fclose(this->outputFile);	
		this->outputFile=NULL;		
	}
	return true;
}

//middle process operation of refilling buffer and writing updated buffer to the output file
int  BWTStateBuffer::flushRefill()
{
	//first - flash what is in buffer to current output file 
	if(this->totalElementsInBuffer>0 )
	{
		//open file for appending
		this->outputFile = fopen (this->outputFileName.c_str(),"ab"); 

		if ( ! this->outputFile )
		{
			displayWarning ("BWTStateBuffer flush-refill writing of output failed: cannot write to output file "+this->outputFileName);
			return 0;
		}
	
		size_t written =  fwrite (this->buffer,sizeof(char),(size_t)(this->totalElementsInBuffer),this->outputFile);
		if(written !=(size_t)this->totalElementsInBuffer)
		{
			displayWarning("OrderBuffer appending of output failed: not all "+T_to_string<uint64>( this->totalElementsInBuffer) +" elements were not written to output file "+this->outputFileName);
			return 0;
		}
		fclose(this->outputFile);	
		this->outputFile=NULL;		
	}

	//try to fill the buffer
	this->inputFile=fopen((this->inputFileName).c_str(),"rb");
	if(!this->inputFile)
	{
		displayWarning("BWTStateBuffer - flash-refill: File "+this->inputFileName+" not found.");
		return RES_FAILURE;
	}	

    int ret=fseeklarge ( this->inputFile , this->currPositionInFile , SEEK_SET);
	if (ret!=0)
		return RES_FAILURE;

	//read from the corresponding file position
	size_t read = fread(this->buffer, sizeof(char),(size_t)this->capacity,this->inputFile);
	if(read>0)
	{
		this->totalElementsInBuffer = read;
		this->curPositionInBuffer = 0;
		this->currPositionInFile += read;
	}
	else //finished file
	{
        fclose (this->inputFile);
	    this->inputFile = NULL;
		return RES_PROCESSED;
	}
	
    fclose (this->inputFile);
	this->inputFile = NULL;
    return RES_SUCCESS;
}

