#include "InputReader.h"

using namespace std;

InputReader::InputReader()
{
	this->processed=false;
}

char InputReader::getSentinel()
{
	return this->alphabet->getSentinel();
}