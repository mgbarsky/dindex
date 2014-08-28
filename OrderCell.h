#ifndef ORDERCELL_H
#define ORDERCELL_H

#define STATE_UNRESOLVED 0
#define STATE_RESOLVED_LEFT 1
#define STATE_RESOLVED_RIGHT_LEFT 2
#define STATE_OUTPUT_RESOLVED 3

class OrderCell //8 bytes  per suffix position - struct reduced size to 4 bytes
{



public:
 static char GET_CHAR_BEFORE_SPLIT [11];
 static char GET_CHAR_AFTER_SPLIT [11];
 static char GET_COMBINED_SPLIT_INDEX [5] [5];

OrderCell() {
    this->state_chunkID=0;
    this->lcp = 0;
    this->charsInfo=0;
}

//last 14 bits of state_chukID
short getChunkID () {
	return (this->state_chunkID & REMOVE_STATE_MASK);
}
    
void setChunkID (short id) {
	//clean bits from previous value
	this->state_chunkID = ((this->state_chunkID & CLEAN_CHUNK_ID_MASK) | id);
}

unsigned char getLCP () { 
	return this->lcp;
}
    
void setLCP (unsigned char val) { 
	this->lcp = val;
}

void incrementLCP () { 
	this->lcp ++;
}

//first 3 bits of chars info
char getResolvingChar () {
	return (this->charsInfo >> 5);
}
    
void setResolvingChar (char val) {
	//move val to the first 3 bits 
	//char movedVal = val << 5;
	//clean bits from prev
	this->charsInfo = ( (this->charsInfo & CLEAN_RESOLVING_CHAR_MASK) | (val << 5));
}

//bits 4-7 of char info contain a combination of split chars
char getSplitCharBefore () {
	//extract value of a combined split
	//for this first clear first 3 bits:  this->charsInfo & CLEAN_RESOLVING_CHAR_MASK
	//then shift 1 to the right ->>1	
	//unsigned char combinedSplitID = (unsigned char) ((this->charsInfo & CLEAN_RESOLVING_CHAR_MASK) >> 1);
	return GET_CHAR_BEFORE_SPLIT[ ((this->charsInfo & CLEAN_RESOLVING_CHAR_MASK) >> 1)];
}
    
void setSplitCharsBeforeAfter (char valBefore, char valAfter) {
	//first, clean combined split area: bits 4-7 : this->charsInfo & CLEAN_COMBINED_SPLIT_MASK
	//unsigned char cleaned = this->charsInfo & CLEAN_COMBINED_SPLIT_MASK;
	//then find out value of a combined split
	//char combined = GET_COMBINED_SPLIT_INDEX [valBefore] [valAfter] ;

	//now move this value one bit to the left
	//combined = combined << 1;

	// put it in place
	 this->charsInfo = (this->charsInfo & CLEAN_COMBINED_SPLIT_MASK) | ( GET_COMBINED_SPLIT_INDEX [(int)valBefore] [(int)valAfter] << 1);	
}

char getSplitCharAfter () {
	//extract value of a combined split
	//for this first clear first 3 bits:  this->charsInfo & CLEAN_RESOLVING_CHAR_MASK
	//then shift 1 to the right ->>1	
	//unsigned char combinedSplitID = ((this->charsInfo & CLEAN_RESOLVING_CHAR_MASK) >> 1);
	return GET_CHAR_AFTER_SPLIT[ ((this->charsInfo & CLEAN_RESOLVING_CHAR_MASK) >> 1)];
}

//first 2 bits of this->state_chunkID
char getState () {
	return (this->state_chunkID >> 14) ;
}
    
void setState (char val) {
	//clean previous value first
	//this->state_chunkID = (this->state_chunkID & REMOVE_STATE_MASK);
	//unsigned short shifted = val << 14;
	this->state_chunkID = ((this->state_chunkID & REMOVE_STATE_MASK) | (val << 14));
}

//complete skip bit - last bit of charInfo


private:
	unsigned short state_chunkID; // 2 bits for a state, 14 bits for chunk id
	unsigned char charsInfo;//3 bits for a resolving char, 4 bits for a split, 1 bit is a total skip flag
	unsigned char lcp; //up to 256 
 //   char nextResolved;
//    char prevResolved;
const static unsigned short MAX_SHORT = ~0;
const static unsigned char MAX_CHAR = ~0;
const static unsigned short REMOVE_STATE_MASK =  MAX_SHORT >> 2;
const static unsigned short CLEAN_CHUNK_ID_MASK = (unsigned short) (MAX_SHORT << (short)14);
const static unsigned char CLEAN_RESOLVING_CHAR_MASK = MAX_CHAR >> 3;
const static unsigned char CLEAN_COMBINED_SPLIT_MASK = MAX_CHAR &  ~(1<<1) &  ~(1<<2) &  ~(1<<3) &  ~(1<<4);
	


} ;


#endif
