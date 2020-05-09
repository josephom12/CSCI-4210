/* 	
	Joseph Om, Spring 2020
	Homework 1 - Files, Strings, and Memory Allocation in C
	Score: 100/100 
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Function to convert a given word into it's hash equivalent
int hash(char* word, int cache_size) {
	
	int sum = 0;
	
	int i;
	for (i = 0; i < strlen(word); i++ ){
		sum += (int) *(word + i);
	}

	return sum % cache_size;
}

int main(int argc, char** argv) {

	// In the case that there are not enough arguments 
	if (argc < 3){
		fprintf(stderr, "ERROR: Invalid arguments\n");
		fprintf(stderr, "USAGE: %s \n", *(argv + 0));
		return EXIT_FAILURE; 
	}

	// In the case that the cache size argument is not an integer 
	int cache_size = atoi(*(argv + 1));

	if (cache_size == 0 && *(argv+1) != 0){
		fprintf(stderr, "ERROR: Cache size argument is not valid\n");
		return EXIT_FAILURE;
	}

	// In the case that the file name argmuent is invalid 




	#ifdef DEBUG_MODE
	printf("%s\n",*(argv + 1));
	printf("%s\n",*(argv + 2));
	#endif

	FILE *fp; 
	fp = fopen(*(argv + 2), "r");

	if (fp == NULL){
		fprintf(stderr, "ERROR: Unable to open the text file\n");
		return EXIT_FAILURE;
	}

	// Create the character array 
	char** cache = calloc(cache_size, sizeof(char*));
	
	// Read every word and input it into the cache
	char* buffer = calloc(128, sizeof(char));
	int ptr = 0;

	while (fp){

		if (feof(fp)) {
			break;
		}

		char temp = fgetc(fp);
		
		if (isalnum(temp)){
			*(buffer+ptr) = temp;
			ptr += 1;
		}
		else {

			*(buffer+ptr) = '\0';

			// If this is a valid word, compute the hash 
			if (strlen(buffer) >= 3){
				int curr_hash = hash(buffer, cache_size);

				// Calloc
				if (*(cache + curr_hash) == 0){
					printf("Word \"%s\" ==> %d (calloc)\n", buffer, curr_hash);

					char* new_entry = calloc(strlen(buffer) + 1, sizeof(char));
					strcpy(new_entry, buffer);

					*(cache + curr_hash) = new_entry;
				}

				// Realloc
				else {
					printf("Word \"%s\" ==> %d (realloc)\n", buffer, curr_hash);	
					*(cache + curr_hash) = (char*) realloc(*(cache + curr_hash), strlen(buffer) + 1);
					strcpy(*(cache + curr_hash), buffer);
				}
			}

			memset(buffer, 0, 128);
			ptr = 0;
		}
	}

	free(buffer);

	// Deallocate all dynamic memory 
	int i;
	for (i = 0; i < cache_size; i++){

		if (*(cache + i) == NULL) {
			continue;
		}

		printf("Cache index %d ==> \"%s\"\n", i, *(cache + i));
		free(*(cache+i));
	}

	free(cache);

	fclose(fp);

	return EXIT_SUCCESS;
}