#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "list.h"

/*
Config file format:
[ENTITY_NAME]
Key=Value
 .
 .
 .
*/

#define CONFIG_MAX_LINE_LEN	256

typedef struct {
	char *key;
	char *value;
} ConfigKeyValuePair;

typedef struct {
	char *name;
	List *keyValuePairs;
} ConfigEntity;

typedef struct {
	List *entities;
} Config;


// Parse the given config file into an in-memory representation
Config * config_file_parse(char * filename);

void config_entity_set_value(ConfigEntity *entity, char *key, char *value);

// Get a value for a given key in an entity
char * config_entity_value(ConfigEntity *entity, char *key);

void config_file_write(char *filename, Config *config);

#endif // _CONFIG_H_
