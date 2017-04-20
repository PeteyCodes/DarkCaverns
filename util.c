/*
 * util.c - Miscellaneous utility fns
 */


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

