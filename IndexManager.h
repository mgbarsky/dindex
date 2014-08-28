#ifndef INDEXCMANAGER_H
#define INDEXCMANAGER_H

#include "IndexConfig.h"
#include <vector>
#include <string>
using namespace std;

class IndexManager
{
	public:		
		void init(IndexConfig *cfg);
		bool buildIndex ();
		bool search(const string * query, vector<int> *result);
		bool scan (const int k, vector<int> *result);
	private:	
		IndexConfig *config;		
};

#endif