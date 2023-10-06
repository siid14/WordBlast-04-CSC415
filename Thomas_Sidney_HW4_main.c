#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MAX_WORD_LENGTH 100
#define MAX_TOTAL_WORDS 100000

// You may find this Useful
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
int numWords = 0; // track snumber of words in the wordCount array

// structure to hold thread-specific data
typedef struct
{
    int fileDescriptor;
    int threadIndex;
    off_t segmentSize;
} ThreadData;

// * FUNCTION
// function to count and tally words in a file segment
void *countWords(void *arg)
{
    ThreadData *threadData = (ThreadData *)arg;
    int fileDescriptor = threadData->fileDescriptor;
    int threadIndex = threadData->threadIndex;
    off_t segmentSize = threadData->segmentSize;

    printf("Thread %d started\n", threadIndex);

    // calculate the start position for this thread
    off_t startPosition = threadIndex * segmentSize;

    printf("Thread %d start position: %lld\n", threadIndex, (long long)startPosition);

    // seek to the start position in the file
    lseek(fileDescriptor, startPosition, SEEK_SET);

    // Print the current file pointer position before reading
    off_t currentPos = lseek(fileDescriptor, 0, SEEK_CUR);
    printf("Thread %d current file pointer position: %lld\n", threadIndex, (long long)currentPos);

    printf("Thread %d seeked to position : %lld\n\n", threadIndex, (long long)startPosition);

    // read the assigned portion of the file into a buffer
    // allocate a buffer of size 'segmentSize' dynamically
    char *buffer = (char *)malloc(segmentSize);
    if (buffer == NULL)
    {
        perror("Error allocating buffer");
        pthread_exit(NULL);
    }

    printf("Segment Size: %lld\n", (long long)segmentSize);
    printf("Buffer size: %lu\n\n", sizeof(buffer));

    ssize_t bytesRead = read(fileDescriptor, buffer, segmentSize);
    // ! apparently sizeof(buffer) returns the size of the pointer
    // if (segmentSize > sizeof(buffer))
    // {
    //     fprintf(stderr, "Segment size is too large for the buffer.\n");
    // }

    // print the new file pointer position after reading
    currentPos = lseek(fileDescriptor, 0, SEEK_CUR);
    printf("Thread %d new file pointer position: %lld\n", threadIndex, (long long)currentPos);

    if (bytesRead < 0)
    {
        perror("Error reading file");
        free(buffer); // free the buffer if there's an error
        pthread_exit(NULL);
    }

    printf("Thread %d read %zd bytes from file\n", threadIndex, bytesRead);

    // Check if the segment size is greater than the bytes actually read
    if (segmentSize > bytesRead)
    {
        fprintf(stderr, "Thread %d: Segment size is larger than bytes read. Expected %lld bytes, but read %zd bytes.\n", threadIndex, (long long)segmentSize, bytesRead);
    }

    // initialize strtok with delimiters
    char *token = strtok(buffer, delim);

    while (token != NULL)
    {
        printf("Thread %d: Token: %s\n", threadIndex, token);

        // TODO: check if 'token' has 6 or more characters and update word counts accordingly
        if (strlen(token) >= 6)
        {
            printf("Thread %d -- %s has 6 characters or more\n", threadIndex, token);

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
        token = strtok(NULL, delim);
    }

    free(buffer);

    printf("Thread %d completed\n", threadIndex);

    pthread_exit(NULL);
}

// compare function for qsort to sort by word counts in descending order
int compareWordCounts(const void *a, const void *b)
{
    const WordCount *word1 = (const WordCount *)a;
    const WordCount *word2 = (const WordCount *)b;

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
    printf("File Name: %s\n", fileName);
    int threadCount = atoi(argv[2]);
    printf("Thread Count: %d\n", threadCount);

    // * FILE OPENING
    printf("--- START READING FILE ---\n");
    // open the file
    int fileDescriptor = open(fileName, O_RDONLY);
    printf("File Descriptor: %d\n", fileDescriptor);
    if (fileDescriptor == -1)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    printf("--- END READING FILE ---\n\n");

    // * FILE SIZE & THREAD SEGMENTATION
    // determine the file size
    off_t fileSize = lseek(fileDescriptor, 0, SEEK_END);
    printf("Size of the file: %lld\n", (long long)fileSize);
    // reset file pointer to beginning of file
    lseek(fileDescriptor, 0, SEEK_SET);

    // calculate the segment size for each thread
    off_t segmentSize = fileSize / threadCount;
    printf("Size for each thread: %lld\n\n", (long long)segmentSize);

    // initialize the mutex
    pthread_mutex_init(&mutex, NULL);

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

    printf("--- START CREATE & START THREADS ---\n");
    // create and start threads
    for (int i = 0; i < threadCount; i++)
    {
        // ensure thread know which file to read
        threadDataArray[i].fileDescriptor = fileDescriptor;
        printf("Thread[%d].fileDescriptor: %d\n", i, fileDescriptor);

        // ensure thread know which file segment to process
        threadDataArray[i].segmentSize = segmentSize;
        printf("Thread[%d].segmentSize: %lld\n", i, (long long)segmentSize);

        // pass the thread index as an argument
        threadDataArray[i].threadIndex = i; // set the thread index

        // create thread to process with countWords function
        pthread_create(&threads[i], NULL, countWords, &threadDataArray[i]);
    }
    printf("--- END CREATE & START THREADS ---\n\n");

    printf("--- START WAIT THREADS TO FINISH ---\n");
    // wait for the threads to finish
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    printf("--- END WAIT THREADS TO FINISH ---\n\n");

    free(threadDataArray);

    // sort the wordCount array
    qsort(wordCount, numWords, sizeof(WordCount), compareWordCounts);

    // * DISPLAY TOP 10 WORDS
    for (int i = 0; i < 10; i++)
    {
        printf("Word: %s, Count: %d\n", wordCount[i].word, wordCount[i].count);
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
