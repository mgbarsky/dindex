#include "IndexManager.h"
//#include "ChunkIndexer.h"
#include "DNALineReader.h"
#include "MergeManager.h"

using namespace std;

void IndexManager::init(IndexConfig*  cfg)
{
	//here sets variables needed for different operations
	this->config = cfg;	
}

//indexes all files into an index
bool IndexManager::buildIndex()
{
	//ChunkIndexer chunkIndexer;
	MergeManager mergeManager;
	DNALineReader reader;

	//if(!chunkIndexer.init(this->config, &reader))
	//	return false;

	//if(!chunkIndexer.indexChunks())
	//	return false;
	
	//int longestLine = (int) chunkIndexer.getLongestInputLine();
	//printf("Longest input line is of size %d\n",longestLine);
	//if(!mergeManager.init(this->config,&reader,chunkIndexer.getTotalChunks()))
	if(!mergeManager.init(this->config,&reader,3))
		return false;

	if(!mergeManager.mergeChunks())
		return false;

	return true;
	
}

bool IndexManager::search(const string * query, vector<int> *result)
{
	return true;
}

bool IndexManager::scan (const int k, vector<int> *result)
{
	return true;
}
