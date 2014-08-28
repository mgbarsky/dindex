#include "InOrderManager.h"

using namespace std;

InOrderManager::InOrderManager() {
    this->buffer = NULL;
}
	
InOrderManager::~InOrderManager() {
    if (this->buffer != NULL)
        delete [] this->buffer;
}

//memory allocation - occurs once per program
//the same array of InOrderCells is used for different purposes
//the same file is used for reading (during iteration)
//and for writing (during bifurcation)
bool InOrderManager::init( uint64 maxCapacityBytes, const std::string &tempDir) {	
    this->totalElementsRead =0;
   
    this->currPositionInBuffer = -1; //no position yet
    
    this->capacity = maxCapacityBytes / sizeof (InOrderCell);
    this->buffer = new InOrderCell[(size_t)this->capacity];

	if (!this->buffer) return false;

	//construct file name for the file used as an input and as an output
	this->fileName = tempDir + separator() + "inorderarray";
	
    //preset buffer for writing mode - open output file
    this->mode =  INORDER_WRITE_MODE;

    if ( !(this->file = fopen (this->fileName.c_str(),"wb"))) {
        displayWarning("InOrderManager - init failed: cannot write to File "+this->fileName+".");
	    return false;
    }
    
    //ready for buffered output
	return true;
}


 //for bifurcation and for initialization
bool InOrderManager::setNextInOrderCell (short chunkID, char resolvingChar) { //write-only mode    
    if (this->currPositionInBuffer == -1) {  //first-time writing
        this->currPositionInBuffer = 0;      
    }

    //write newCell to the current position
    this->buffer[this->currPositionInBuffer].setChunkID (chunkID);
    this->buffer[this->currPositionInBuffer].setResolvingChar (resolvingChar);

    this->currPositionInBuffer++;
    if ((uint64)this->currPositionInBuffer == this->capacity)
    {
        if (!this->flush())
            return false;
    }
    
    return true;   
}

//for iteration - important to know that all input cells are read - so returns result
int InOrderManager::getNextInOrderCell (InOrderCell *pointerToResultCell) { //read only mode	
    int ret = RES_SUCCESS;
    if (this->currPositionInBuffer == -1 || (uint64)this->currPositionInBuffer == this->totalElementsRead) {  //first-time reading or next-time refill
        ret = this->refill();
        if(ret!=RES_SUCCESS)
	        return ret;        
    }
    
    *pointerToResultCell = this->buffer[this->currPositionInBuffer];
    this->currPositionInBuffer++;
    return ret;
}


int InOrderManager::refill () {
    int ret = RES_SUCCESS;    

    //read as much as you can from the current position in file        
    size_t read = fread (this->buffer, sizeof(InOrderCell),(size_t)this->capacity,this->file);
    if(read > 0)	{
	    this->totalElementsRead = read;
	    this->currPositionInBuffer = 0;
    }
    else {       
        // no more elements to process
            return RES_PROCESSED;       
    }

    return ret;
}

bool InOrderManager::flush () {
    uint64 totalToWrite = this->currPositionInBuffer;

    if (totalToWrite <=0) //nothing to write - buffer is empty
        return true;   

    //write totalToWrite 
	size_t written =  fwrite (this->buffer,sizeof(InOrderCell),(size_t)(totalToWrite),this->file);
	if(written !=(size_t)totalToWrite) {
		displayWarning("InOrderManager - appending of output failed: not all "+T_to_string<uint64>( totalToWrite) +" elements were written to output file "+this->fileName);
		return false;
	}	

	return true;
}

bool InOrderManager::wrapUp () { //depends on mode
    if (this->mode == INORDER_WRITE_MODE ) {//need to flush remaining in the buffer    
        if (!this->flush())
            return RES_FAILURE;
    }
 
    //close file
    fclose (this->file);
    this->file = NULL;
    return RES_SUCCESS;
}

//resets input file pointer for the next iteration
bool InOrderManager::nextIterationReset() {
    this->mode = INORDER_READ_MODE;
    this->currPositionInBuffer = -1;
    this->totalElementsRead =0;        
       
    if ( !(this->file = fopen (this->fileName.c_str(),"rb"))) {
        displayWarning("InOrderManager - nextIterationReset failed: File "+this->fileName+" cannot be found.");
        return false;
    }

	return true;
}

//resets output file pointer for the next bifurcation
bool InOrderManager::nextBifurcationReset() {
    this->mode = INORDER_WRITE_MODE;
    this->currPositionInBuffer = -1;
    this->totalElementsRead =0;   

    //initialize output file
    if ( !(this->file = fopen (this->fileName.c_str(),"wb"))) {
        displayWarning("InOrderManager - nextBifurcationReset failed: cannot write to File "+this->fileName+".");
	    return false;
    }
       
	return true;
}
