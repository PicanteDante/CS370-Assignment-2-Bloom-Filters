#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

/*
 * For baller performance, use:
 * BLOOM_SIZE 481221388
 # HASH_COUNT 23
 */

#define BLOOM_SIZE 206237738
#define HASH_COUNT 10
#define MAX_LINE_LENGTH 256
#define HASH_TABLE_SIZE 16777216  // 2^24

typedef struct {
	unsigned char *array;
} BloomFilter;

typedef struct {
	int true_positive;
	int true_negative;
	int false_positive;
	int false_negative;
} Results;

typedef struct WordEntry {
	char *word;
	struct WordEntry *next;
} WordEntry;

WordEntry *hash_table[HASH_TABLE_SIZE] = {NULL};

// Bloom filter functions
void bloom_init(BloomFilter *filter) {
	filter->array = calloc(BLOOM_SIZE / 8, 1);
	if (filter->array == NULL) {
		fprintf(stderr, "Failed to allocate memory for Bloom filter\n");
		exit(1);
	}
}

void bloom_free(BloomFilter *filter) {
	free(filter->array);
}

unsigned int hash(const unsigned char *str, unsigned int seed) {
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, str, strlen((char *)str));
	MD5_Update(&ctx, &seed, sizeof(seed));
	MD5_Final(digest, &ctx);
	
	return *(unsigned int*)digest;
}

void bloom_add(BloomFilter *filter, const char *str) {
	for (int i = 0; i < HASH_COUNT; i++) {
		unsigned int index = hash((unsigned char *)str, i) % BLOOM_SIZE;
		filter->array[index / 8] |= 1 << (index % 8);
	}
}

int bloom_check(BloomFilter *filter, const char *str) {
	for (int i = 0; i < HASH_COUNT; i++) {
		unsigned int index = hash((unsigned char *)str, i) % BLOOM_SIZE;
		if (!(filter->array[index / 8] & (1 << (index % 8)))) {
			return 0;
		}
	}
	return 1;
}

// Hash table functions
unsigned int hash_string(const char *str) {
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)str, strlen(str), digest);
	return *(unsigned int*)digest % HASH_TABLE_SIZE;
}

void add_word(const char *word) {
	unsigned int index = hash_string(word);
	WordEntry *new_entry = malloc(sizeof(WordEntry));
	new_entry->word = strdup(word);
	new_entry->next = hash_table[index];
	hash_table[index] = new_entry;
}

int check_word(const char *word) {
	unsigned int index = hash_string(word);
	WordEntry *entry = hash_table[index];
	while (entry != NULL) {
		if (strcmp(entry->word, word) == 0) {
			return 1;
		}
		entry = entry->next;
	}
	return 0;
}

void free_hash_table() {
	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
		WordEntry *entry = hash_table[i];
		while (entry != NULL) {
			WordEntry *temp = entry;
			entry = entry->next;
			free(temp->word);
			free(temp);
		}
	}
}

// Main function
int main() {
	BloomFilter filter;
	bloom_init(&filter);
	Results results = {0};

	// Load rockyou.txt into Bloom filter and hash table
	FILE *rockyou = fopen("rockyou.ISO-8859-1.txt", "r");
	if (rockyou == NULL) {
		fprintf(stderr, "Failed to open rockyou.ISO-8859-1.txt\n");
		return 1;
	}

	char line[MAX_LINE_LENGTH];
	while (fgets(line, sizeof(line), rockyou)) {
		line[strcspn(line, "\n")] = 0;
		bloom_add(&filter, line);
		add_word(line);
	}
	fclose(rockyou);

	// Process dictionary.txt
	FILE *dictionary = fopen("dictionary.txt", "r");
	if (dictionary == NULL) {
		fprintf(stderr, "Failed to open dictionary.txt\n");
		bloom_free(&filter);
		free_hash_table();
		return 1;
	}

	while (fgets(line, sizeof(line), dictionary)) {
		line[strcspn(line, "\n")] = 0;
		int bloom_result = bloom_check(&filter, line);
		int actual_present = check_word(line);

		if (bloom_result) {
			printf("maybe\n");
			if (actual_present) results.true_positive++;
			else results.false_positive++;
		} else {
			printf("no\n");
			if (actual_present) results.false_negative++;
			else results.true_negative++;
		}
	}
	fclose(dictionary);

	// Print statistics
	printf("True Positives: %d\n", results.true_positive);
	printf("True Negatives: %d\n", results.true_negative);
	printf("False Positives: %d\n", results.false_positive);
	printf("False Negatives: %d\n", results.false_negative);

	// Clean up! Clean up! Everybody, Everywhere!
	// Clean up! Clean up! Everybody do your share!
	bloom_free(&filter);
	free_hash_table();

	return 0;
}
