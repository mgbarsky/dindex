#ifndef UTILS_H
#define UTILS_H

#include <typeinfo>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>

//#ifdef __cplusplus
//extern "C" {
//#endif

#ifdef _WIN32
	#define  int64 __int64 
	#define  uint64 unsigned __int64
	#define  int32 __int32
	#define  uint32 unsigned __int32
#else
	#include <stdint.h>
	#define  int64 int64_t
	#define  uint64 uint64_t
	#define  int32 int32_t
	#define  uint32 uint32_t
#endif

#ifndef ABS
# define ABS(_a) (((_a) < 0) ? (-_a) : (_a))
#endif /* ABS */

#ifndef MIN
# define MIN(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#endif /* MIN */
#ifndef MAX
# define MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#endif /* MAX */

void exitWithError(const std::string &error) ;
void displayWarning(const std::string &warning) ;

//define execution result constants
#define RES_FAILURE 0
#define RES_SUCCESS 1
#define RES_PROCESSED 2

template <typename T>
std::string T_to_string(T const &val) 
{
    std::ostringstream ostr;
    ostr << val;

    return ostr.str();
}

template <typename T>
T string_to_T(std::string const &val) 
{
    std::istringstream istr(val);
    T returnVal;
    if (!(istr >> returnVal))
	    exitWithError("TypeConvertor: Not a valid " + (std::string)typeid(T).name() + " received!\n");

    return returnVal;
}

//this reads either entire file or part of it into an array of strings
uint64 readLinesIntoArray (const std::string *fileName, std::vector<std::string> *result, 
			const size_t firstLine=0, const size_t maxLines=0);

inline char separator()
{
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

inline int fseeklarge(FILE * fp, uint64 offset, int origin)
{
#ifdef _WIN32
    return _fseeki64( fp, (__int64) offset,  origin);
#else
    return fseeko64(fp, (off64_t) offset,  origin);
#endif
}

inline int64 ftelllarge(FILE * fp)
{
#ifdef _WIN32
    return _ftelli64( fp);
#else
    return (int64)ftello64(fp);
#endif
}

template <typename ValueType>
	int64 flushData(std::vector<ValueType>& output, uint64 howMany, const std::string &fileName)
{
	FILE *file;
		
	file=fopen((fileName).c_str(), "wb");
	if (!file)
	{
		displayWarning("Utils flushData: Cannot write to file " + fileName);
		return -1;
	}

	size_t written = fwrite(&(output[0]), sizeof(ValueType),(size_t)howMany,file);

	if(written != howMany)
	{
		std::string wantedSTR = T_to_string<uint64>(howMany);
		std::string writtenSTR = T_to_string<uint64>(written);
		displayWarning("Utils flushData: Not all has been written. wanted to write "+wantedSTR+ " but written only "+writtenSTR);
		return -1;
	}
	fclose(file);
	return (int64)written;

}


//#ifdef __cplusplus
//}
//#endif

#endif
