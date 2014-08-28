CC = g++
CFLAGOPT = -O3 -Wall 
CFLAGS = -D_LARGEFILE_SOURCE
CFLAGS += -fno-exceptions
CFLAGS += -finline-functions
CFLAGS += -funroll-loops
CFLAGS += -fpermissive
CFLAGS += -pthread
CFLAGOFFSET = -D_FILE_OFFSET_BITS=64
LDFLAGS=-lz
MATHFLAG=-lm

# Source files
INDEX_CREATE_SRC=divsufsort.c Utils.cpp Alphabet.cpp DNAAlphabet.cpp InputReader.cpp DNAReader.cpp DNALineReader.cpp IndexConfig.cpp ChunkIndexer.cpp BWTStateBuffer.cpp OrderCell.cpp OrderStateInputBuffer.cpp OrderStateOutputBuffer.cpp OrderStateReadWriteBuffer.cpp MergeManager.cpp IndexManager.cpp buildSA.cpp

# Binaries
all: indexfull 

indexfull: $(INDEX_CREATE_SRC)
	$(CC) $(CFLAGOPT) $(CFLAGOFFSET) $(CFLAGS) $^ -o $@ 
clean:  
	rm indexfull
