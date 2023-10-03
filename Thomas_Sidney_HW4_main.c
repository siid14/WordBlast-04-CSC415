#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

// You may find this Useful
char *delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

//  mutex for thread synchronization
pthread_mutex_t mutex;

// structure to hold word counts
typedef struct
{
    char word[100];
    int count;
} WordCount;

// * pointer of the data for the threads to process
// structure to hold thread-specific data
typedef struct
{
    int fileDescriptor;
    off_t segmentSize;
} ThreadData;

// function to count and tally words in a file segment
void *countWords(void *arg)
{
    ThreadData *threadData = (ThreadData *)arg;
    // thread-specific code to count words
    // make sure to lock the mutex before updating shared data structures

    pthread_mutex_lock(&mutex);
    // perform word counting and tallying here

    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
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
    printf("--- START READING FILE ---\n");
    // open the file
    int fileDescriptor = open(fileName, O_RDONLY);
    if (fileDescriptor == -1)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    printf("--- END READING FILE ---\n");

    // * FILE SIZE & THREAD SEGMENTATION
    // determine the file size
    off_t fileSize = lseek(fileDescriptor, 0, SEEK_END);
    printf("Size of the file: %lld\n", (long long)fileSize);
    // reset file pointer to beginning of file
    lseek(fileDescriptor, 0, SEEK_SET);

    // calculate the segment size for each thread
    off_t segmentSize = fileSize / threadCount;
    printf("Size for each thread: %lld\n", (long long)segmentSize);

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
        threadDataArray[i].fileDescriptor = fileDescriptor;
        threadDataArray[i].segmentSize = segmentSize;
        pthread_create(&threads[i], NULL, countWords, &threadDataArray[i]);
    }
    printf("--- END CREATE & START THREADS ---\n");

    printf("--- START WAIT THREADS TO FINISH ---\n");
    // wait for the threads to finish
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }
    printf("--- END WAIT THREADS TO FINISH ---\n");

    // * DISPLAY TOP 10 WORDS
    // TODO: Process TOP 10 and display

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
    free(threadDataArray);
}
