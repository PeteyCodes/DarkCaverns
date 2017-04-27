#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdint.h>

char * String_Create(const char * stringWithFormat, ...);
char * String_Append(char *string, const char * stringWithFormat, ...);
void String_Destroy(char *string);

char * String_Substring (int32_t startIndex, uint32_t len);
char * String_Trim(char *string);
//String_Split (return List of strings)
//String_Join

int32_t String_Compare(char *string1, char *string2);

bool String_StartsWith(char *string, char *prefix);
bool String_EndsWith(char *string, char *suffix);
bool String_Contains(char *string, char *searchString);
int32_t String_IndexOf(char *string, char *searchString);

#endif
