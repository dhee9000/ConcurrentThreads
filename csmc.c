#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 5

void *StudentThread(void *data)
{
	//DEBUG PRINT STATEMENT
	printf("Student Thread Running!\n");
	pthread_exit(NULL);
}

void *TutorThread(void *data)
{
	pthread_exit(NULL);
}

void *CoordinatorThread(void *data)
{
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int NUM_STUDENTS, NUM_TUTORS; NUM_CHAIRS, NUM_HELP;
	
	if(argc ==  5){
		NUM_STUDENTS = atoi(argv[1]);
		NUM_TUTORS = atoi(argv[2]);
		NUM_CHAIRS = atoi(argv[3]);
		NUM_HELP = atoi(argv[4]);
	}
	else {
		printf("ERROR! Not Enough Arguments! Syntax: CSMC #students #tutors #chairs #help");
		exit(-1);
	}

	pthread_t threads[NUM_THREADS];
	int rc; long t;
	for(t = 0; t < NUM_THREADS; t++){
		// DEBUG PRINT STATEMENT
		printf("MAIN: Creating Student Thread #%ld\n", t);
		rc = pthread_create(&threads[t], NULL, StudentThread, (void *)t);
		if(rc) {
			//DEBUG PRINT STATEMENT
			printf("MAIN ERROR: Error Creating Thread! Code: %d\n", rc);
			exit(-1);
		}
	}

	pthread_exit(NULL);
}
