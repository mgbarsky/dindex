#include "ChunkIndexer.h"
#include "InputReader.h"
#include "DNALineReader.h"
#include "divsufsort.h"

using namespace std;

ChunkIndexer::~ChunkIndexer()
{
	//free memory of arrays used - TBD test what really happens
	vector<unsigned char>().swap(this->chunk);
	vector<int>().swap(this->chunkSA);
}

bool ChunkIndexer::init(IndexConfig *cfg, InputReader *reader)
{
	this->longestInputLine=0;
	this->reader = reader;

	//check that we have all the required information
	string fofilenames;
	string fokeys;
	if(cfg->keyExists("fofilenames"))
	{
		fofilenames = cfg->getValue<string>("fofilenames");		
	}
	else
	{
		displayWarning("ChunkIndexer: Init failed. The name of the file (fofilenames) containing input file names is missing in db config");
		return false;
	}

	if(cfg->keyExists("fokeys"))
	{
		fokeys = cfg->getValue<string>("fokeys");
	}
	else
	{
		displayWarning("ChunkIndexer: Init failed. The name of the file (fokeys) containing sample names is missing in db config");
		return false;
	}

	//now actually read the above files into string arrays
	uint64 readLines = readLinesIntoArray(&fofilenames,&(this->inputFileNames));
	if(readLines <= 0)
	{
		displayWarning("ChunkIndexer: Init failed. Error reading file "+fofilenames+": probably does not contain any lines");
		return false;
	}

	readLines = readLinesIntoArray(&fokeys,&(this->sampleKeys));
	if(readLines <= 0)
	{
		displayWarning("ChunkIndexer: Init failed. The file with sample keys "+fokeys+" does not contain any lines");
		return false;
	}

	//get config parameters and fill default values if params are missing
	this->chunkMaxCharacters = 250000000;
	if(cfg->keyExists("chunkMaxCharacters"))
	{
		this->chunkMaxCharacters = cfg->getValue<uint64>("chunkMaxCharacters");
	}

	this->maxChunks = 32000;
	if(cfg->keyExists("maxChunks"))
	{
		this->maxChunks = cfg->getValue<short>("maxChunks");
	}

	this->maxFileSize = 4096000 ;//4MB default
	if(cfg->keyExists("maxFileSize"))
	{
		this->maxFileSize = cfg->getValue<uint32>("maxFileSize");
	}

	string fileName = "mapChunkToSample.csv";
	if(cfg->keyExists("mappingFileName"))
	{
		fileName = cfg->getValue<string>("mappingFileName");
	}

	this->dir = ".";
	if(cfg->keyExists("metaDataDir"))
	{
		this->dir = cfg->getValue<string>("metaDataDir");
	}

	this->mappingFileName = this->dir + separator() + fileName;

	this->tempDir = ".";
	if(cfg->keyExists("tempDir"))
	{
		this->tempDir = cfg->getValue<string>("tempDir");
	}

	this->bwtFileExt =".bwt";
	if(cfg->keyExists("bwtFileExt"))
	{
		this->bwtFileExt = cfg->getValue<string>("bwtFileExt");
	}

	this->saFileExt = ".sa";
	if(cfg->keyExists("saFileExt"))
	{
		this->saFileExt = cfg->getValue<string>("saFileExt");
	}

	this->chunkFileExt=".chunk";
	if(cfg->keyExists("chunkFileExt"))
	{
		this->chunkFileExt = cfg->getValue<string>("chunkFileExt");
	}
	//we need to allocate a char * and an int * buffer to use with Mori suffix sorting algo
	
	this->chunk.resize((size_t)(this->chunkMaxCharacters+2)); //for sentinel and char termination character
	
	this->chunkSA.resize((size_t)this->chunkMaxCharacters);

	//all values are set, can start indexing
	return true;
}

bool ChunkIndexer::indexChunks ()
{
	//go in a loop through input files, read an appropriate chunk from this file into array of lines
	//build BWT using in-mem suffix sorting algo and write it to disk
	//also add to mapping array: index in this array corresponds to - chunk id, value - to sample id	
	this->sentinel = this->reader->getSentinel();
	this->currentChunkID=0;	

	short sampleID=0;
	for(vector<string>::const_iterator it = this->inputFileNames.begin(); it != this->inputFileNames.end(); ++it,++sampleID)     
	{
		string fname=*it;
		this->reader->init(fname,0);

		this->currentInputPos=0;
		this->totalAddedCharacters=0;

		uint64 currentPosInFile=0L;

		if(this->currentChunkID >= this->maxChunks)
		{					
			displayWarning("ChunkIndexer: Number of chunks "+ T_to_string<short>(this->currentChunkID)+ " exceeded predefined max " +T_to_string<short>(this->maxChunks));
			//write mappings from chunk ID to sample into a file
			this->recordChunkMapping();
			//stop processing the rest of files
			return true;
		}
		string line= reader->nextValidToken(&currentPosInFile);
		while(line!="")
		{  
			if(line!="skip") //valid line
			{
				if(line.size() > this->longestInputLine)
					this->longestInputLine=line.size();
				//did not reach max characters yet - add current line to chunk
				if ( (uint64)(this->totalAddedCharacters+line.size() +2)>= (uint64)this->chunkMaxCharacters)
				{
					if(!this->finalizeCurrentChunk(sampleID))
						return false;

					//start over a new chunk
					this->currentInputPos=0;
					this->totalAddedCharacters=0;				

					this->currentChunkID++;					
				}

				//transfer characters in inified format to the current input chunk
				unsigned int s=0;
				for(; s< line.size(); s++)
				{
					this->chunk[(size_t)this->currentInputPos] = this->reader->getUnifiedCharacter(line.at(s));
					this->chunkSA[(size_t)this->currentInputPos]=0;
					this->currentInputPos++;
					this->totalAddedCharacters++;
				}
				//add sentinel character at the end of the line
				this->chunk[(size_t)this->currentInputPos] = this->sentinel;
		        this->currentInputPos++;
				this->totalAddedCharacters++;	

				this->chunkSA[(size_t)this->currentInputPos]=0;							
			}
			
			line= reader->nextValidToken(&currentPosInFile);
		}
			
		//reading lines from the file is done	
        this->reader->closeFile(); //finished adding characters from this file
        
		//check if something is still in memory and flush it as the current chunk
		if(this->totalAddedCharacters>0)
		{
			if(!this->finalizeCurrentChunk(sampleID))
				return false;
			this->currentChunkID++;
								
			//start over a new chunk
			this->currentInputPos=0;
			this->totalAddedCharacters=0;				
		}	
		
        printf("Finished processing file %s (file id = %d), total chunks so far=%d\n",fname.c_str(),sampleID,this->currentChunkID);
	}
		
	//record mappings from chunk IDs to samples into a file
	if(!this->recordChunkMapping())
		return false;	

	return true;
}

bool ChunkIndexer::finalizeCurrentChunk(short fileID)
{
	this->chunkToSampleMap.push_back(fileID); //finished chunk, record the file it belongs to
				
	//add final sentinel character
	this->chunk[(size_t)this->currentInputPos] = this->sentinel;
	this->currentInputPos++;
	this->totalAddedCharacters++;
					
	this->chunk[(size_t)this->currentInputPos]='\0'; //terminate input char array - just in case TBD check
	
	//build SA from already collected characters
	if(!this->buildSAMori())
	{
		displayWarning("Build suffix array failed \n");
		return false; 
	}
	
	//record BWT simple, SA simple and entire chunk as it was indexed
	if(!this->recordSA(this->currentChunkID)) return false;  
	if(!this->recordBWT(this->currentChunkID)) return false;
	if(!this->recordChunk(this->currentChunkID)) return false;
	return true;
}

bool ChunkIndexer::buildSAMori()
{
	if(divsufsort(&(this->chunk[0]),&(this->chunkSA[0]),(int)this->totalAddedCharacters) != 0)
	{
		displayWarning("ChunkIndexer: Can not index chunk of data. Errpr occured in building suffix array by Mori algorithm");
		return false;
	}
	return true;
}

bool ChunkIndexer::recordSA(short chunkID)
{	
	string saFileName = this->dir + separator() + T_to_string<short>(chunkID)+this->saFileExt;
	
	uint64 written = flushData<int>(this->chunkSA,this->totalAddedCharacters, saFileName);
	if(written != this->totalAddedCharacters)
		return false;
	return true;
}

bool ChunkIndexer::recordChunk(short chunkID) //tbd test if binary is a good idea
{	
	string chunkFileName = this->dir + separator() + T_to_string<short>(chunkID)+this->chunkFileExt;
	
	uint64 written = flushData<unsigned char>(this->chunk,this->totalAddedCharacters, chunkFileName);
	if(written != this->totalAddedCharacters)
		return false;
	return true;
}

//this is a basic version that writes only BWT characters needed for merge
//change - it also records small chunks of BWTStateCell arrays - needed for efficient merge
//TBD - use different writers to write a searchable compressed BWT
bool ChunkIndexer::recordBWT(short chunkID)
{
	string bwtFileName = this->dir + separator() + T_to_string<short>(chunkID)+this->bwtFileExt;
	
	ofstream textWriter;
	textWriter.open(bwtFileName.c_str());

	if(!textWriter)
	{
		displayWarning("ChunkIndexer::recording BWT: cannot write to file " + bwtFileName);
		return false;
	}

	for(unsigned int i=0;i<this->totalAddedCharacters;i++)
	{
		int pos = this->chunkSA[i];
		if(pos==0)
		{
			textWriter<<this->sentinel;
		}
		else
		{
			textWriter << this->chunk[pos-1];
		}		
	}

	textWriter.close();

	return true;
}


bool ChunkIndexer::recordChunkMapping()
{
	short totalChunks=this->currentChunkID;
	
	ofstream textWriter;

	textWriter.open(this->mappingFileName.c_str());

	if(!textWriter)
	{
		displayWarning("ChunkIndexer::recording sample mapping: cannot write to file " + this->mappingFileName);
		return false;
	}

	for(int i=0;i<totalChunks;i++)
	{
		string chunkID = T_to_string<short>(i);
		string fileID = T_to_string<short>(this->chunkToSampleMap[i]);
        string inputKey = this->sampleKeys[this->chunkToSampleMap[i]];
		string inputFileName = this->inputFileNames[this->chunkToSampleMap[i]];
		
		textWriter<<chunkID+","+ fileID +","+inputKey+","+ inputFileName<<endl ;		
	}

	textWriter.close();
	return true;
}

short ChunkIndexer::getTotalChunks()
{
	return this->currentChunkID;
}
