#include "IndexConfig.h"
#include "IndexManager.h"

int main(int argc, char *argv[])
{
	//loads configuration values
	IndexConfig cfg  (argv[1]);
    cfg.load();

	//Initialize Index manager
	IndexManager manager;

	manager.init(&cfg);

	if(!manager.buildIndex())
		return 1;
	
	return 0;
}