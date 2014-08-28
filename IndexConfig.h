#ifndef INDEXCONFIG_H
#define INDEXCONFIG_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include "Utils.h"

class IndexConfig
{
    public:
        //constructor
        IndexConfig(const std::string &fName);
	   
        //loads content of a config file into variables
	    void load();	   

        bool keyExists(const std::string &key) const;
	   
        template <typename ValueType>
	    ValueType getValue(const std::string &key, ValueType const &defaultValue = ValueType()) const
        {
            if (!this->keyExists(key))
	            return defaultValue;

            return string_to_T<ValueType>(this->dictionary.find(key)->second);
        }

     private:
	    std::map<std::string, std::string> dictionary;  //stores key-value pairs
	    std::string fName;  //configuration file name

        //parsing functions
        //removes everything after comment sign #, including #
	    void removeComment(std::string &line) const;	   

        //answers a question if line is empty
	    bool onlyWhitespace(const std::string &line) const;

        //answers question if the line is of valid format key=value
	    bool validLine(const std::string &line) const;	    

        //extracts first trimmed (no-space) substring from line - before =
	    void extractKey(const std::string &line, size_t const &sepPos, std::string &key  ) const;
	   
        //extracts the entire substring after = and up to the end of the line (comments are supposedly removed)
	    void extractValue( const std::string &line, size_t const &sepPos, std::string &value) const;	    

        //extracts key-value pair from a line with the valid format - and adds it to the dctionary
	    void extractContents(const std::string &line) ;
	  
        //checks validity of a line and if valid extracts key-value by calling extractContents
	    void parseLine(const std::string &line, size_t const lineNo);	   
};

#endif
