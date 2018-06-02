/*
 * util.c - Miscellaneous utility fns
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typedefs.h"
#include "util.h"
#include "String.h"


#define PREFIX_COUNT 10
#define SUFFIX_COUNT 10

char *fnamePrefix[] = {"Gar","Ben","Arg","Tar","Mar","Gen","Zor","Raf","Bur","Kor"};
char *fnameSuffix[] = {"ath","amm","erth","ogan","'ul","ew","xor","tar","ith","elon"};
char *lnamePrefix[] = {"Sword","Axe","Stone","Gold","Light","Warg","Pike","Star","Moon","Sun"};
char *lnameSuffix[] = {"bringer","crusher","smith","slinger","smiter","hexer","caster","rider","horn","grinder"};

char * name_create() {
	i32 idx1 = rand() % PREFIX_COUNT;
	i32 idx2 = rand() % SUFFIX_COUNT;
	i32 idx3 = rand() % PREFIX_COUNT;
	i32 idx4 = rand() % SUFFIX_COUNT;
	char *name = String_Create("%s%s %s%s", fnamePrefix[idx1], fnameSuffix[idx2], 
											lnamePrefix[idx3], lnameSuffix[idx4]);

	return name;
}


/* Determine the endianness of the system we're running on. */
bool system_is_little_endian() {
    unsigned int x = 0x76543210;
    char *c = (char*) &x; 
    if (*c == 0x10) { return true; }
    return false;
}



/* Convert a character string representing a hex value to an int */
int xtoi(char *hexstring) {
	int	i = 0;
	
	if ((*hexstring == '0') && (*(hexstring+1) == 'x')) {
		  hexstring += 2;
	}

	while (*hexstring) {
		char c = toupper(*hexstring++);
		if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A'))) {
			break;
		}
		c -= '0';
		if (c > 9) {
			c -= 7;
		}
		i = (i << 4) + c;
	}

	return i;
}

