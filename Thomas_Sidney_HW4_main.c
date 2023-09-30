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

int main(int argc, char *argv[])
{
    // * LOOK FOR ARGUMENTS
    // check if user enter the correct nnumbers of arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <FileName> <ThreadCount>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // parse command-line arguments
    char *fileName = argv[1];
    int threadCount = atoi(argv[2]);

    // * OPEN FILE
    printf("--- START READING FILE ---\n");
    // open the file
    int fileDescriptor = open(fileName, O_RDONLY);
    if (fileDescriptor == -1)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    printf("--- END READING FILE ---\n");

    // * DIVIDE BY THREADS
    // determine the file size
    off_t fileSize = lseek(fileDescriptor, 0, SEEK_END);
    printf("Size of the file: %lld\n", (long long)fileSize);
    // reset file pointer to beginning of file
    lseek(fileDescriptor, 0, SEEK_SET);

    // calculate the segment size for each thread
    off_t segmentSize = fileSize / threadCount;
    printf("Size for each thread: %lld\n", (long long)segmentSize);

    //***TO DO***  Look at arguments, open file, divide by threads
    //             Allocate and Initialize and storage structures

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    // *** TO DO ***  start your thread processing
    //                wait for the threads to finish

    // ***TO DO *** Process TOP 10 and display

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

    // ***TO DO *** cleanup
    close(fileDescriptor);
}
