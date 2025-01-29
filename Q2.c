#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>

#define MAX_WORD_LENGTH 100
#define HASH_TABLE_SIZE 10000

// Structure to store word frequencies
typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordFrequency;

// Global variables
int total_word_count = 0;
int vowel_word_count = 0;
WordFrequency frequency_table[HASH_TABLE_SIZE];
int unique_word_count = 0;
pthread_mutex_t lock;

// Function to check if a word starts with a vowel
int starts_with_vowel(const char *word) {
    if (word[0] == 'a' || word[0] == 'e' || word[0] == 'i' || word[0] == 'o' || word[0] == 'u' ||
        word[0] == 'A' || word[0] == 'E' || word[0] == 'I' || word[0] == 'O' || word[0] == 'U') {
        return 1;
    }
    return 0;
}

// Function to update the frequency table
void update_frequency_table(const char *word) {
    pthread_mutex_lock(&lock); // Lock to protect shared data

    // Check if the word already exists in the frequency table
    for (int i = 0; i < unique_word_count; i++) {
        if (strcmp(frequency_table[i].word, word) == 0) {
            frequency_table[i].count++;
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    // If the word is new, add it to the frequency table
    if (unique_word_count < HASH_TABLE_SIZE) {
        strcpy(frequency_table[unique_word_count].word, word);
        frequency_table[unique_word_count].count = 1;
        unique_word_count++;
    }

    pthread_mutex_unlock(&lock); // Unlock after updating
}

// Thread function to process a chunk of the file
void *process_chunk(void *arg) {
    char *chunk = (char *)arg;
    char word[MAX_WORD_LENGTH];
    int chunk_word_count = 0;
    int chunk_vowel_count = 0;

    // Tokenize the chunk into words
    char *token = strtok(chunk, " \t\n\r");
    while (token != NULL) {
        // Remove punctuation and convert to lowercase
        int len = strlen(token);
        for (int i = 0; i < len; i++) {
            if (ispunct(token[i])) {
                memmove(&token[i], &token[i + 1], len - i);
                len--;
                i--;
            } else {
                token[i] = tolower(token[i]);
            }
        }

        if (strlen(token) > 0) {
            // Update word count
            chunk_word_count++;

            // Update vowel word count
            if (starts_with_vowel(token)) {
                chunk_vowel_count++;
            }

            // Update frequency table
            update_frequency_table(token);
        }

        token = strtok(NULL, " \t\n\r");
    }

    // Update global counters
    pthread_mutex_lock(&lock);
    total_word_count += chunk_word_count;
    vowel_word_count += chunk_vowel_count;
    pthread_mutex_unlock(&lock);

    return NULL;
}

// Function to compare word frequencies for sorting
int compare_frequencies(const void *a, const void *b) {
    return ((WordFrequency *)b)->count - ((WordFrequency *)a)->count;
}

int main() {
    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Open the input file
    FILE *file = fopen("Q2.txt", "r");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the entire file into memory
    char *file_content = (char *)malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    // Split the file into chunks for threads
    int num_threads = 4; // You can adjust the number of threads
    long chunk_size = file_size / num_threads;
    pthread_t threads[num_threads];
    char *chunk_start = file_content;

    // Create threads to process chunks
    for (int i = 0; i < num_threads; i++) {
        char *chunk_end = chunk_start + chunk_size;
        if (i < num_threads - 1) {
            // Ensure the chunk ends at a word boundary
            while (*chunk_end != ' ' && *chunk_end != '\t' && *chunk_end != '\n' && *chunk_end != '\r') {
                chunk_end++;
            }
        } else {
            chunk_end = file_content + file_size;
        }

        // Create a thread to process the chunk
        pthread_create(&threads[i], NULL, process_chunk, chunk_start);
        chunk_start = chunk_end + 1;
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Sort the frequency table to find the top 10 words
    qsort(frequency_table, unique_word_count, sizeof(WordFrequency), compare_frequencies);

    // Print results
    printf("Total word count: %d\n", total_word_count);
    printf("Words starting with vowels: %d\n", vowel_word_count);
    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10 && i < unique_word_count; i++) {
        printf("%s: %d\n", frequency_table[i].word, frequency_table[i].count);
    }

    // Cleanup
    free(file_content);
    pthread_mutex_destroy(&lock);

    return 0;
}