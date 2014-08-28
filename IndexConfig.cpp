#include "IndexConfig.h"

  
//Private
//parsing functions
//removes everything after comment sign #, including #
void IndexConfig::removeComment(std::string &line) const
{
    if (line.find('#') != line.npos)
	    line.erase(line.find('#'));
}

//answers a question if line is empty
bool IndexConfig::onlyWhitespace(const std::string &line) const
{
    return (line.find_first_not_of(' ') == line.npos);
}

//answers question if the line is of valid format key=value
bool IndexConfig::validLine(const std::string &line) const
{
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    if (temp[0] == '=')
	    return false;

    for (size_t i = temp.find('=') + 1; i < temp.length(); i++)
	    if (temp[i] != ' ')
		    return true;

    return false;
}

//extracts first trimmed (no-space) substring from line - before =
void IndexConfig::extractKey(const std::string &line, size_t const &sepPos, std::string &key  ) const
{
    key = line.substr(0, sepPos);
    if (key.find('\t') != line.npos || key.find(' ') != line.npos)
	    key.erase(key.find_first_of("\t "));
}

//extracts the entire substring after = and up to the end of the line (comments are supposedly removed)
void IndexConfig::extractValue( const std::string &line, size_t const &sepPos, std::string &value) const
{
    value = line.substr(sepPos + 1);
    value.erase(0, value.find_first_not_of("\t "));
    value.erase(value.find_last_not_of("\t ") + 1);
}

//extracts key-value pair from a line with the valid format - and adds it to the dctionary
void IndexConfig::extractContents(const std::string &line) 
{
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    size_t sepPos = temp.find('=');

    std::string key, value;
    this->extractKey(temp, sepPos, key);
    this->extractValue(temp, sepPos, value);

    if (!this->keyExists(key))
	    this->dictionary.insert(std::pair<std::string, std::string>(key, value));
    else
	    exitWithError("IndexConfig: Can only have unique key names! Key "+key+" already in the dictionary \n");
}

//checks validity of a line and if valid extracts key-value by calling extractContents
void IndexConfig::parseLine(const std::string &line, size_t const lineNo)
{
    if (line.find('=') == line.npos)
	    exitWithError("IndexConfig: Couldn't find separator on line: " + T_to_string(lineNo) + "\n");

    if (!validLine(line))
	    exitWithError("IndexConfig: Bad format for line: " + T_to_string(lineNo) + "\n");

    extractContents(line);
}

    //public:
//constructor
IndexConfig::IndexConfig(const std::string &fName)
{
    this->fName = fName;
}

//loads content of a config file into variables
void IndexConfig::load()
{
    std::ifstream file;
    file.open(this->fName.c_str());
    if (!file)
	    exitWithError("IndexConfig: File " + this->fName + " couldn't be found!\n");

    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line))
    {
	    lineNo++;
	    std::string temp = line;

	    if (temp.empty())
		    continue;

	    IndexConfig::removeComment(temp);
	    if (IndexConfig::onlyWhitespace(temp))
		    continue;

	    this->parseLine(temp, lineNo);
    }

    file.close();
    
}

bool IndexConfig::keyExists(const std::string &key) const
{
    return this->dictionary.find(key) != dictionary.end();
}



