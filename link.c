#include "link.h"

struct LinkStruct{
    int identifier;
    char key[3], modo; //modo - flag para saber se foi de um find ou de um efind
    Link *next;
};

Link *initLink(void){
    return NULL;
}

void freeLink(Link *node){
   if (node==NULL)
      return;

   freeLink(node->next);
   free(node);
}

int search_identifier(Link **first, int identifier, char *modo){
    Link *t = (*first);
    for(t = (*first); t != NULL; t = t->next){
       if(t->identifier == identifier ){
           identifier = key_check(t->key);
           *(modo) = t->modo;
           *(first) = removeLink(*(first),t->identifier);
           return identifier;
       }
    }
    return -1;
}

Link *insertUnsortedLink(Link *next, int id, char key[3], char modo)
{
    Link *new;

    new = (Link *) malloc(sizeof(Link));

    if(new == NULL)
        return NULL;

    new->identifier = id;
    strcpy(new->key,key);
    new->modo = modo;
    new->next = next;

    return new;
}

Link *removeLink(Link *first, int id){
	Link *next, *aux = first;

	if(aux->identifier == id){
		next = aux->next;
		first = next;
		free(aux);
		return first;
	}

	for(aux = first; aux->next != NULL; aux = aux->next){
		next = aux->next;
		if(next->identifier == id){
			aux->next = next->next;
			free(next);
			return first;
		}
	}

    return first;
}
