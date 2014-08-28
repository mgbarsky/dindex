#ifndef INORDERMANAGER_H
#define INORDERMANAGER_H

#include "Utils.h"
#include "InOrderCell.h"
#include "DNALineReader.h"

//this is used for reading input order during iteration
//and for writing new output order (chopped) during bifurcation
//it works with a single file, and as such file pointers remain open
#define INORDER_WRITE_MODE 0
#define INORDER_READ_MODE 1

class InOrderManager
{
public:
	InOrderManager() ;
	InOrderManager(const InOrderManager& a) {}
    ~InOrderManager();

	bool init (uint64 maxCapacityBytes, 	const std::string &tempDir); //memory allocation, and the initial mode is set to WRITE_ONLY - to write initial values
	
    //for bifurcation
	bool setNextInOrderCell (short chunkID, char resolvingChar) ; //write-only mode	   

	//for iteration - important to know that all inoput cells are read - so returns result
	int getNextInOrderCell (InOrderCell *pointerToResultCell) ; //read only mode        

	bool nextIterationReset();
	bool nextBifurcationReset();
	
    bool wrapUp ();  

private:
    int refill();
    bool flush();

    int mode;
	std::string fileName;
    FILE *file;	
    uint64 totalElementsRead;
    
    uint64 capacity;

    int64 currPositionInBuffer;
	InOrderCell *buffer;		
};


#endif
