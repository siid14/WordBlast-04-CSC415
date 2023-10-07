/**************************************************************
 * Class:  CSC-415-01 - Fall 2023
 * Name: Sidney Thomas
 * Student ID: 918656419
 * GitHub ID: siid14
 * Project: Assignment 4 – Word Blast
 *
 * File: Thomas_Sidney_HW4_main.c
 *
 * Description: This program counts word frequencies in a text file
 *              using multiple threads and displays the top 10 words
 *              with 6 characters or more.
 *
 **************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define MAX_WORD_LENGTH 100
#define MAX_TOTAL_WORDS 100000

// delimiters for tokenization
char *delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

//  mutex for thread synchronization
pthread_mutex_t mutex;

// * DATA STRUCTURES
// structure to hold word counts
typedef struct
{
    char word[MAX_WORD_LENGTH];
    int count;
} WordCount;

WordCount wordCount[MAX_TOTAL_WORDS];
int numWords = 0; // track number of words in the wordCount array

// structure to hold thread-specific data
typedef struct
{
    int fileDescriptor;
    int threadIndex;
    off_t segmentSize;
    char *buffer; // add a buffer for strtok_r
} ThreadData;

// * FUNCTIONS
// function to count and tally words in a file segment
void *countWords(void *arg)
{

    // extract thread-specific data from the argument
    ThreadData *threadData = (ThreadData *)arg;
    int fileDescriptor = threadData->fileDescriptor;
    int threadIndex = threadData->threadIndex;
    off_t segmentSize = threadData->segmentSize;
    char *buffer = threadData->buffer;

    // calculate the start position for this thread
    off_t startPosition = threadIndex * segmentSize;

    // seek to the start position in the file
    lseek(fileDescriptor, startPosition, SEEK_SET);

    // off_t currentPos = lseek(fileDescriptor, 0, SEEK_CUR);

    // read the assigned portion of the file into a buffer
    ssize_t bytesRead = read(fileDescriptor, buffer, segmentSize);

    // currentPos = lseek(fileDescriptor, 0, SEEK_CUR);

    if (bytesRead < 0)
    {
        fprintf(stderr, "Error: Failed to read from file: %s\n", strerror(errno));
        pthread_exit(NULL);
    }

    // check if the segment size is greater than the bytes actually read
    // if (segmentSize > bytesRead)
    // {
    //     fprintf(stderr, "Thread %d: Segment size is larger than bytes read. Expected %lld bytes, but read %zd bytes.\n", threadIndex, (long long)segmentSize, bytesRead);
    // }

    // initialize strtok_r with delimiters for tokenisation
    char *token;
    char *saveptr;

    token = strtok_r(buffer, delim, &saveptr);

    while (token != NULL)
    {

        if (strlen(token) >= 6)
        {
            // lock mutex before any data shared
            pthread_mutex_lock(&mutex);

            // search for 'token' in the wordCount array
            int found = 0;
            for (int i = 0; i < numWords; i++)
            {
                if (strcmp(wordCount[i].word, token) == 0)
                {
                    // 'token' already exists in the array, increment its count
                    wordCount[i].count++;
                    found = 1;
                    break;
                }
            }

            // if 'token' is not found in the array, add it
            if (!found && numWords < MAX_TOTAL_WORDS)
            {
                strcpy(wordCount[numWords].word, token);
                wordCount[numWords].count = 1;
                numWords++;
            }

            // unlock the mutex after updating shared data
            pthread_mutex_unlock(&mutex);
        }

        // get the next token
        token = strtok_r(NULL, delim, &saveptr);
    }

    pthread_exit(NULL); // exit the thread
}

// compare function for qsort to sort by word counts in descending order
int compareWordCounts(const void *a, const void *b)
{
    // cast the arguments (pointers) to WordCount structures
    const WordCount *word1 = (const WordCount *)a;
    const WordCount *word2 = (const WordCount *)b;

    // compare the counts of the two WordCount structures
    // and return a negative value if word2 should come before word1
    // return a positive value if word1 should come before word2
    // and return 0 if they have equal counts (no change in order)
    return word2->count - word1->count;
}

int main(int argc, char *argv[])
{
    // * ARGUMENTS VALIDATION
    // check if user enter the correct numbers of arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <FileName> <ThreadCount>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // parse command-line arguments
    char *fileName = argv[1];
    int threadCount = atoi(argv[2]);

    // * FILE OPENING
    // open the file
    int fileDescriptor = open(fileName, O_RDONLY);
    if (fileDescriptor == -1)
    {
        fprintf(stderr, "Failed to open file '%s' for reading: %s\n", fileName, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // * FILE SIZE & THREAD SEGMENTATION
    // determine the file size
    off_t fileSize = lseek(fileDescriptor, 0, SEEK_END);

    // reset file pointer to beginning of file
    lseek(fileDescriptor, 0, SEEK_SET);

    // calculate the segment size for each thread
    off_t segmentSize = fileSize / threadCount;

    // initialize the mutex
    pthread_mutex_init(&mutex, NULL);
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        fprintf(stderr, "Error: Failed to initialize mutex: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // array to hold thread IDs
    pthread_t threads[threadCount];

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************

    // * THREADS CREATION & EXECUTION
    ThreadData *threadDataArray = malloc(threadCount * sizeof(ThreadData));

    // create and start threads
    for (int i = 0; i < threadCount; i++)
    {
        // ensure thread know which file to read
        threadDataArray[i].fileDescriptor = fileDescriptor;

        // ensure thread know which file segment to process
        threadDataArray[i].segmentSize = segmentSize;

        // pass the thread index as an argument
        threadDataArray[i].threadIndex = i; // set the thread index

        // allocate a buffer for each thread
        threadDataArray[i].buffer = (char *)malloc(segmentSize);
        if (threadDataArray[i].buffer == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory for buffer in thread %d: %s\n", i, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // create thread to process with countWords function
        pthread_create(&threads[i], NULL, countWords, &threadDataArray[i]);
        if (pthread_create(&threads[i], NULL, countWords, &threadDataArray[i]) != 0)
        {
            fprintf(stderr, "Error: Failed to create thread %d: %s\n", i, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // wait for the threads to finish
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(threads[i], NULL);
        // if (pthread_join(threads[i], NULL) != 0)
        // {
        //     fprintf(stderr, "Error: Failed to join thread %d: %s\n", i, strerror(errno));
        // }
    }

    free(threadDataArray);

    // free buffers after threads finish
    for (int i = 0; i < threadCount; i++)
    {
        free(threadDataArray[i].buffer);
    }

    // sort the wordCount array
    qsort(wordCount, numWords, sizeof(WordCount), compareWordCounts);

    printf("\n\n");

    printf("Word Frequency Count on %s with %d threads\n", fileName, threadCount);
    // * DISPLAY TOP 10 WORDS
    for (int i = 0; i < 10; i++)
    {
        printf("Number %d is %s with a count of %d\n", i + 1, wordCount[i].word, wordCount[i].count);
    }

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
    {
        --sec;
        n_sec = n_sec + 1000000000L;
    }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    // * FREE ALLOCATED MEMORY
    close(fileDescriptor);
    pthread_mutex_destroy(&mutex);
}
