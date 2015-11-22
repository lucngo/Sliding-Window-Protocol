#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>
#include "common.h"

//Linked list functions
int ll_get_length(LLnode *);
void ll_append_node(LLnode **, void *);
LLnode * ll_pop_node(LLnode **);
void ll_destroy_node(LLnode *);

//Print functions
void print_cmd(Cmd *);

//Time functions
long timeval_usecdiff(struct timeval *, 
                      struct timeval *);

//TODO: Impelemt these functions
char * convert_frame_to_char(Frame *);
Frame * convert_char_to_frame(char *);

//CRC stuff
char get_bit(char byte, int pos);
char crc8(char* array, int byte_len);
void append_crc (char * array, int array_len);
int is_corrupted (char * array, int array_len);

//Splitting Messages
void ll_split_head (LLnode ** head_ptr, int payload_size);
#endif
