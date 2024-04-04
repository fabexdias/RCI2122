#ifndef RING
#define RING

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "verify.h"

#include "link.h"

#define LCL 0
#define SUCC 1
#define SHRC 2


typedef struct _Node Node;
typedef struct _Intruders Intruders;

void node_cpy(Node *, Node *);
void node_setup(Node *, char *, char *, char *, int);
int key_check(char *);
int create_socket(struct addrinfo *,struct addrinfo **, Node , char *, char *);
int send_msg(int *, const char *);
int recv_ack(int);
void distance_calculator(char* , char* , char* , char* , int* );



#endif