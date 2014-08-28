#ifndef RUNWRITER_H
#define RUNWRITER_H

#include "Utils.h"
#include "Alphabet.h"
#include "HeaderCell.h"

class RunWriter {
public:
    bool init (const std::string &textFileName, const std::string &suffixArrayFileName, 
        unsigned short offset, short numMultiples8, const std::string &runFileName );
    bool extractRun (HeaderCell nextSkip );

private:
    FILE *inputFile;
    FILE *outputFile;
    std::string textFileName;
    std::string saFileName;
    std::string runFileName;

};


#endif