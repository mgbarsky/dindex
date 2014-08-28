#ifndef INPUTORDERCELL_H
#define INPUTORDERCELL_H

class InOrderCell { //2 bytes per suffix position - only for alphabet upto 8 (including sentinel)
public:
    InOrderCell () {
        _rchar_chunkID =0;
    }
    //last 13 bits of resolving_char_chunkID
    short getChunkID () {
	    return (_rchar_chunkID & REMOVE_RCHAR_MASK);
    }
        
    void setChunkID (short id) {
	    //clean bits from previous value
	    _rchar_chunkID = ((_rchar_chunkID & CLEAN_CHUNK_ID_MASK) | id);
    }

    //first 3 bits of resolving_char_chunkID
    char getResolvingChar () {
	    return (_rchar_chunkID >> 13);
    }
        
    void setResolvingChar (char val) {
	    //move val to the first 3 bits of 2-byte unsigned short
        //(ZERO_SHORT | val ) << 13 	
	    //clean bits from prev and write new bits
	    _rchar_chunkID = ( (_rchar_chunkID & CLEAN_RESOLVING_CHAR_MASK ) | ((ZERO_SHORT | val ) << 13));
    }

private:
	unsigned short _rchar_chunkID; // 3 bits for resolving, 13 bits for chunk id	
    const static unsigned short ZERO_SHORT = 0;
    const static unsigned short MAX_SHORT = ~0;
    const static unsigned short REMOVE_RCHAR_MASK =  MAX_SHORT >> 3;
    const static unsigned short CLEAN_CHUNK_ID_MASK = (unsigned short) (MAX_SHORT << (short)13);
    const static unsigned short CLEAN_RESOLVING_CHAR_MASK = (MAX_SHORT >> 3);
} ;

#endif
