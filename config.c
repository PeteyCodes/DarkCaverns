/*
 * config.c
 */

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
Config * config_file_parse(char * filename) {
	Config *cfg = NULL;

	// Open config file
	FILE * configFile = fopen(filename, "r");
	if (configFile) {
		cfg = (Config *)malloc(sizeof(Config));
		cfg->entities = list_new(free);
		char buffer[CONFIG_MAX_LINE_LEN];

		ConfigEntity *currentEntity = NULL;

		// Loop through each line of the file
		while (fgets(buffer, CONFIG_MAX_LINE_LEN, configFile) != NULL) {
			if (buffer[0] == '[') {
				// New entity - grab the entity name
				char *entityName = strtok(buffer, "[]");	// grabs everything between brackets

				// Create a new entity structure
				ConfigEntity *entity = (ConfigEntity *)malloc(sizeof(ConfigEntity));
				char *copy = malloc(strlen(entityName) + 1); 
				strcpy(copy, entityName);
				entity->name = copy;
				entity->keyValuePairs = list_new(free);
				list_insert_after(cfg->entities, NULL, entity);
				currentEntity = entity;

			} else if ((buffer[0] != '\n') && (buffer[0] != ' ')) {
				// If we have key/value data, parse it into its various parts and add the pair to the entity.
				buffer[CONFIG_MAX_LINE_LEN-1] = '\0';	// Ensure that our buffer string is null-terminated
				char *key = strtok(buffer, "=");
				char *value = strtok(NULL, "\n");
				if ((key != NULL) && (value != NULL)) {
					ConfigKeyValuePair *kv = (ConfigKeyValuePair *)malloc(sizeof(ConfigKeyValuePair));
					char *copyKey = malloc(strlen(key) + 1); 
					strcpy(copyKey, key);
					kv->key = copyKey;
					char *copyValue = malloc(strlen(value) + 1); 
					strcpy(copyValue, value);
					kv->value = copyValue;

					list_insert_after(currentEntity->keyValuePairs, NULL, kv);					
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
	// TODO - add a new key-value pair to the entity
}


