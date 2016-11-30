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
		char buffer[CONFIG_MAX_LINE_LEN];

		// Loop through each line of the file
		while (fgets(buffer, CONFIG_MAX_LINE_LEN, configFile) != NULL) {
			if (buffer[0] == '[') {
				// New entity - grab the entity name
				char *entityName = strtok(buffer, "[");
				entityName = strtok(NULL, "]");		// grabs everything between brackets

				// Create a new entity structure
				// TODO

			} else {
				// If we have key/value data, parse it into its various parts and add the pair to the entity.
				// TODO
			}
		}
	}

	return cfg;
}

ConfigEntity * config_get_entity(Config * cfg, char * entityName) {
	// TODO
}

// TODO: fn to get a value for a given key in an entity



