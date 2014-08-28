#include "OrderCell.h"


/**
combinations of split chars
0 - 00 (no split), 1 - 01, 2 -02, 3 - 03, 4 - 04,
5 - 12, 6 - 13, 7- 14, 
8 - 23, 9 - 24, 10 - 34
**/
char OrderCell::GET_CHAR_BEFORE_SPLIT [] = {
	0,0,0,0,0,
	1,1,1,
	2,2,3
};

char OrderCell::GET_CHAR_AFTER_SPLIT [] = {
	0,1,2,3,4,
	2,3,4,
	3,4,4
};

char OrderCell::GET_COMBINED_SPLIT_INDEX [5] [5] = {{0,1,2,3,4},{0,0,5,6,7},{0,0,0,8,9},{0,0,0,0,10},{0,0,0,0,0}};



