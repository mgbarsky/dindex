#include "Utils.h"

void exitWithError(const std::string &error) 
{
	std::cout << error;
	std::cin.ignore();
	std::cin.get();

	exit(EXIT_FAILURE);
}

void displayWarning(const std::string &warning)
{
	std::cout << warning << std::endl;
}


uint64 readLinesIntoArray (const std::string *fileName, std::vector<std::string> *result, 
	const size_t firstLine, const size_t maxLines)
{
	std::ifstream file;
    file.open((*fileName).c_str());
    if (!file)
	{
	    displayWarning("InputReader: File " + (*fileName) + " could not be found!");
		return -1;
	}

    std::string line;
    size_t lineNo = 0;
	while (lineNo < firstLine && getline(file, line))
	{
		lineNo++;
	}
	
	lineNo=0;
    while (getline(file, line))
    {
		result->push_back(line); // Add the line to the end
		lineNo++;
		if(maxLines >0 && lineNo == maxLines)
			break;
	}

	file.close();
	return lineNo; //number of lines processed
}
