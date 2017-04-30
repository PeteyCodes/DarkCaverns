#include <stdarg.h>     // va_start, et al.
#include <stdio.h>      // vsnprintf
#include <stdlib.h>
#include <string.h>

#include "String.h"


char * String_Create(const char * stringWithFormat, ...) {
    char *str = NULL;
    char *fmt;

    // Make a copy of our formatted string to work with
    if (stringWithFormat != NULL) {
        fmt = strdup(stringWithFormat);
    } else {
        fmt = strdup("");
    }

    // Now apply the formatting on a trial run to determine how long the formatted string should be
    va_list argp;
    va_start(argp, stringWithFormat);
    char one_char[1];
    int len = vsnprintf(one_char, 1, fmt, argp);
    if (len < 1) {
        return NULL;
    }
    va_end(argp);

    // Allocate enough memory, and generate the formatted string for reals
    str = malloc(len + 1);
    if (!str) {
        return NULL;
    }
    va_start(argp, stringWithFormat);
    vsnprintf(str, len + 1, fmt, argp);
    va_end(argp);

    free(fmt);

    return str;
}

char * String_Append(char *string, const char * stringWithFormat, ...) {
    char *stringToAppend = NULL;
    char *fmt;

    // Make a copy of our formatted string to work with
    if (stringWithFormat != NULL) {
        fmt = strdup(stringWithFormat);
    } else {
        fmt = strdup("");
    }

    // Now apply the formatting on a trial run to determine how long the formatted string should be
    va_list argp;
    va_start(argp, stringWithFormat);
    char one_char[1];
    int len = vsnprintf(one_char, 1, fmt, argp);
    if (len < 1) {
        stringToAppend = "";
    }
    va_end(argp);

    // Allocate enough memory, and generate the formatted string for reals
    stringToAppend = malloc(len + 1);
    if (!stringToAppend) {
        stringToAppend = "";
    }
    va_start(argp, stringWithFormat);
    vsnprintf(stringToAppend, len + 1, fmt, argp);
    va_end(argp);

    free(fmt);

    // Allocate enough memory and concatenate the two strings
    int32_t totalLength = strlen(string) + strlen(stringToAppend) + 1;
    char *combinedString = malloc(totalLength);
    strcpy(combinedString, string);
    strcat(combinedString, stringToAppend);

    return combinedString;    
}

void String_Destroy(char *string) {
    free(string);
}

char * String_Substring (int32_t startIndex, uint32_t len);
char * String_Trim(char *string);
//String_Split (return List of strings)
//String_Join

bool String_Equals(char *string1, char *string2) {
    return (strcmp(string1, string2) == 0);
}

int32_t String_Compare(char *string1, char *string2);

bool String_StartsWith(char *string, char *prefix);
bool String_EndsWith(char *string, char *suffix);
bool String_Contains(char *string, char *searchString);
int32_t String_IndexOf(char *string, char *searchString);

