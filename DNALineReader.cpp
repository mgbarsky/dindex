#include "DNALineReader.h"

using namespace std;

DNALineReader::DNALineReader(): DNAReader() 
{
}

void DNALineReader::closeFile()
{
    this->file.close();
}
bool DNALineReader::init(const std::string &mfileName, uint64 offsetStart)
{
	InputReader::setProcessed(false);
    this->fileName = mfileName;
	this->fileOffset = offsetStart;
    
    this->file.open(this->fileName.c_str());
    if (!this->file)
	{
	    displayWarning("DNALineReader: Input file " + this->fileName + " could not be found!");
		return false;
	}

	//now move to the offset
	//first - how many total bytes?
	this->file.seekg(0,this->file.end);
	this->totalBytes = (uint64) this->file.tellg();
	
	if(this->fileOffset>=this->totalBytes)
	{
		InputReader::setProcessed(true);
		return true;
	}

	//move to the offset starting from the beginning
	this->file.seekg(this->fileOffset,this->file.beg);

	//ready to process next chunk
	return true;
}

string DNALineReader::nextValidToken(uint64 *currentFilePosition)
{
	if(InputReader::isProcessed())
	{
		return "";
	}

	string rawline;

	if(getline(this->file, rawline))
	{
		//determine current file pointer
		*currentFilePosition = (uint64) this->file.tellg();
		if(this->validLine(rawline))				
			return rawline;		
		else
			return "skip";
	}
	else
	{
		InputReader::setProcessed(true);
		return "";
	}
	return "";
}
