#ifndef _LINK_H_
#define _LINK_H_

#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include "verify.h"

typedef struct LinkStruct Link;

/*************LINK**************/

/*OPERATIONS********************/
Link * initLink(void);
Link * insertUnsortedLink(Link * next, int V, char key[3],char modo);
Link * removeLink(Link *first, int V);
int search_identifier(Link **first,int identifier,char *modo);
void freeLink(Link * first);

#endif
