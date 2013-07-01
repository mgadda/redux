//
//  hash.c
//  redux
//
//  Created by Matt Gadda on 22/07/12.
//  Copyright (c) 2012 Catalpa Labs. All rights reserved.
//

#include <stdio.h>
//#include "hash.h"

//  dan bernstein's djb2 via http://www.cse.yorku.ca/~oz/hash.html 
unsigned long hash(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;
	
	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	
	return hash;
}
