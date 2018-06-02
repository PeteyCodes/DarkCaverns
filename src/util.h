#ifndef _UTIL_H_
#define _UTIL_H_

/* 
Generate a "fantasy" name 
*/
char * name_create();

bool system_is_little_endian();

/*
Convert a character string representing a hex number to its integer value
*/
int xtoi(char *hexstring);

#endif // _UTIL_H_
