#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

typedef struct {
    unsigned char *bit_array;
    size_t size;
    int hash_count;
} BloomFilter;

BloomFilter* create_bloom_filter(size_t size, int hash_count) {
    BloomFilter *filter = (BloomFilter*)malloc(sizeof(BloomFilter));
    if (!filter) {
        perror("Error allocating memory for BloomFilter");
        exit(EXIT_FAILURE);
    }
    filter->size = size;
    filter->hash_count = hash_count;
    filter->bit_array = (unsigned char*)calloc((size + 7) / 8, sizeof(unsigned char)); // Ensure we allocate enough bytes
    if (!filter->bit_array) {
        perror("Error allocating memory for bit array");
        free(filter);
        exit(EXIT_FAILURE);
    }
    return filter;
}

unsigned int hash_md5(const char *str, int seed) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    char temp_str[256];
    snprintf(temp_str, sizeof(temp_str), "%s%d", str, seed);
    MD5((unsigned char*)temp_str, strlen(temp_str), digest);
    unsigned int hash = 0;
    for (int i = 0; i < 4; ++i) {
        hash = (hash << 8) | digest[i];
    }
    return hash;
}

void add_to_bloom_filter(BloomFilter *filter, const char *item) {
    for (int i = 0; i < filter->hash_count; ++i) {
        unsigned int hash = hash_md5(item, i);
        size_t index = hash % filter->size;
        filter->bit_array[index / 8] |= (1 << (index % 8));
    }
}

int check_bloom_filter(BloomFilter *filter, const char *item) {
    for (int i = 0; i < filter->hash_count; ++i) {
        unsigned int hash = hash_md5(item, i);
        size_t index = hash % filter->size;
        if (!(filter->bit_array[index / 8] & (1 << (index % 8)))) {
            return 0;
        }
    }
    return 1;
}

void load_bloom_filter(const char *file_path, BloomFilter *filter) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        add_to_bloom_filter(filter, line);
        // printf("Loaded: %s\n", line); // Debug statement
    }
    fclose(file);
}

typedef struct {
    int true_positive;
    int true_negative;
    int false_positive;
    int false_negative;
} Results;

void test_rockyou(const char *file_path, BloomFilter *filter, Results *results, const char **dictionary_words, size_t dictionary_count) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        int is_in_dictionary = 0;
        for (size_t i = 0; i < dictionary_count; ++i) {
            if (strcmp(dictionary_words[i], line) == 0) {
                is_in_dictionary = 1;
                break;
            }
        }
        if (check_bloom_filter(filter, line)) {
            if (is_in_dictionary) {
                results->true_positive++;
                printf("maybe\n");
            } else {
                results->false_positive++;
                printf("maybe\n");
            }
        } else {
            if (is_in_dictionary) {
                results->false_negative++;
                printf("no\n");
            } else {
                results->true_negative++;
                printf("no\n");
            }
        }
    }
    fclose(file);
}

int main() {
    const char *dictionary_file = "dictionary.txt";
    const char *rockyou_file = "rockyou.ISO-8859-1.txt";

    BloomFilter *bloom_filter = create_bloom_filter(139836133, 7); // Updated size and hash_count
    load_bloom_filter(dictionary_file, bloom_filter);

    // Load dictionary words into memory for accurate testing
    FILE *dictionary_fp = fopen(dictionary_file, "r");
    if (!dictionary_fp) {
        perror("Error opening dictionary file");
        exit(EXIT_FAILURE);
    }
    char **dictionary_words = malloc(sizeof(char*) * 13984613);
    if (!dictionary_words) {
        perror("Error allocating memory for dictionary words");
        fclose(dictionary_fp);
        free(bloom_filter->bit_array);
        free(bloom_filter);
        exit(EXIT_FAILURE);
    }
    size_t dictionary_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), dictionary_fp)) {
        line[strcspn(line, "\r\n")] = 0;
        dictionary_words[dictionary_count] = strdup(line);
        if (!dictionary_words[dictionary_count]) {
            perror("Error duplicating string");
            fclose(dictionary_fp);
            free(bloom_filter->bit_array);
            free(bloom_filter);
            for (size_t i = 0; i < dictionary_count; ++i) {
                free(dictionary_words[i]);
            }
            free(dictionary_words);
            exit(EXIT_FAILURE);
        }
        dictionary_count++;
    }
    fclose(dictionary_fp);

    Results results = {0, 0, 0, 0};
    test_rockyou(rockyou_file, bloom_filter, &results, (const char **)dictionary_words, dictionary_count);

    printf("True Positives: %d\n", results.true_positive);
    printf("True Negatives: %d\n", results.true_negative);
    printf("False Positives: %d\n", results.false_positive);
    printf("False Negatives: %d\n", results.false_negative);

    // Free allocated memory
    for (size_t i = 0; i < dictionary_count; ++i) {
        free(dictionary_words[i]);
    }
    free(dictionary_words);
    free(bloom_filter->bit_array);
    free(bloom_filter);

    return 0;
}
