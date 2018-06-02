/*
 * config.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"

// Parse the given config file into an in-memory representation
Config * config_file_parse(char * filename) {
	Config *cfg = NULL;

	// Open config file
	FILE * configFile = fopen(filename, "r");
	if (configFile) {
		cfg = (Config *)calloc(1, sizeof(Config));
		cfg->entities = list_new(free);
		char buffer[CONFIG_MAX_LINE_LEN];

		ConfigEntity *currentEntity = NULL;

		// Loop through each line of the file
		while (fgets(buffer, CONFIG_MAX_LINE_LEN, configFile) != NULL) {
			if (buffer[0] == '[') {
				// New entity - grab the entity name
				char *entityName = strtok(buffer, "[]");	// grabs everything between brackets

				// Create a new entity structure
				ConfigEntity *entity = (ConfigEntity *)calloc(1, sizeof(ConfigEntity));
				char *copy = calloc(strlen(entityName) + 1, sizeof(char)); 
				strcpy(copy, entityName);
				entity->name = copy;
				entity->keyValuePairs = list_new(free);
				list_insert_after(cfg->entities, list_tail(cfg->entities), entity);
				currentEntity = entity;

			} else if ((buffer[0] != '\n') && (buffer[0] != ' ')) {
				// If we have key/value data, parse it into its various parts and add the pair to the entity.
				buffer[CONFIG_MAX_LINE_LEN-1] = '\0';	// Ensure that our buffer string is null-terminated
				char *key = strtok(buffer, "=");
				char *value = strtok(NULL, "\n");
				if ((key != NULL) && (value != NULL)) {
					ConfigKeyValuePair *kv = (ConfigKeyValuePair *)calloc(1, sizeof(ConfigKeyValuePair));
					char *copyKey = calloc(strlen(key) + 1, sizeof(char)); 
					strcpy(copyKey, key);
					kv->key = copyKey;
					char *copyValue = calloc(strlen(value) + 1, sizeof(char)); 
					strcpy(copyValue, value);
					kv->value = copyValue;

					list_insert_after(currentEntity->keyValuePairs, list_tail(currentEntity->keyValuePairs), kv);					
				}

			} else {
				// Blank line - just ignore it
			}
		}
	}

	return cfg;
}

// ConfigEntity * config_get_entity(Config * cfg, char * entityName) {
// 	// TODO
// }

// Get a value for a given key in an entity
char * config_entity_value(ConfigEntity *entity, char *key) {
	ListElement *e = list_head(entity->keyValuePairs);
	while (e != NULL) {
		ConfigKeyValuePair *kv = (ConfigKeyValuePair *)e->data;
		if (strcmp(key, kv->key) == 0) {
			return kv->value;
		}
		e = list_next(e);
	}
	
	return NULL;
}

void config_entity_set_value(ConfigEntity *entity, char *key, char *value) {
	// Add a new key-value pair to the entity

	if (entity->keyValuePairs == NULL) {
		entity->keyValuePairs = list_new(free);
	}

	ConfigKeyValuePair *kv = calloc(1, sizeof(ConfigKeyValuePair));
	kv->key = strdup(key);
	kv->value = strdup(value);

	list_insert_after(entity->keyValuePairs, list_tail(entity->keyValuePairs), kv);
}

void config_file_write(char *filename, Config *config) {

	FILE * configFile = fopen(filename, "w");
	if (configFile) {
		ListElement *e = list_head(config->entities);
		while (e != NULL) {
			ConfigEntity *entity = (ConfigEntity *)e->data;
			fprintf(configFile, "[%s]\n", entity->name);

			ListElement *le = list_head(entity->keyValuePairs);
			while (le != NULL) {
				ConfigKeyValuePair *kv = (ConfigKeyValuePair *)le->data;
				fprintf(configFile, "%s=%s\n", kv->key, kv->value);

				le = list_next(le);
			}

			e = list_next(e);
		}
	}
	fclose(configFile);
}
