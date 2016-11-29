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
	char * key;
	char * value;
} KeyValuePair;

typedef struct {
	List *keyValuePairs;
} ConfigEntity;

typedef struct {
	List *entities;
} Config;


// Parse the given config file into an in-memory representation
Config * config_file_parse(char * filename) {
	Config *cfg = NULL;

	// Open config file
	FILE * configFile = fopen(filename, 'r');
	if (configFile) {
		char buffer[CONFIG_MAX_LINE_LEN];
		// Loop through each line of the file
		while (fgets(buffer, CONFIG_MAX_LINE_LEN, configFile) != NULL) {
			// If we have a new entity, create it

			// If we have key/value data, parse it into its various parts and add the pair to the entity.
			
		}
	}

	return cfg;
}

