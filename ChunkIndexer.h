#ifndef CHUNKINDEXER_H
#define CHUNKINDEXER_H

#include "IndexConfig.h"
#include "InputReader.h"

class ChunkIndexer
{
public:
	ChunkIndexer(){}
	ChunkIndexer(ChunkIndexer& a) {}
	~ChunkIndexer();
	bool init(IndexConfig *cfg, InputReader *reader);
	bool indexChunks();
	short getTotalChunks();
	uint64 getLongestInputLine(){return this->longestInputLine;}
		
private:
	std::vector<std::string> inputFileNames;
	std::vector<std::string> sampleKeys;
	
	uint64 chunkMaxCharacters;
	short maxChunks;
	uint32 maxFileSize;  //chunk size is small - it will never be more than 4 gb
	std::string mappingFileName;
	std::string bwtFileExt;
	std::string saFileExt;
	std::string chunkFileExt;
	InputReader *reader;
	//OutputWriter writer;
	std::vector<unsigned char> chunk;
	std::vector<int> chunkSA; //though it may be that the chunk length is bigger than 4-byte int, we use int here because it is required by Mori algorithm
	uint64 currentInputPos;
	uint64 totalAddedCharacters;
	std::vector<short> chunkToSampleMap;
	short currentChunkID;
	bool buildSAMori();
	bool recordSA(short chunkID);
	bool recordBWT(short chunkID);
	bool recordChunk (short chunkID);
	char sentinel;
	std::string dir;
	std::string tempDir;
	bool recordChunkMapping();
	
	bool finalizeCurrentChunk(short fileID);
	uint64 longestInputLine;
};

#endif