#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

void *StudentThread(void *data)
{
	//DEBUG PRINT STATEMENT
	printf("Student Thread Running!\n");
	pthread_exit(NULL);
}

void *TutorThread(void *data)
{
	printf("Student Thread Running!\n");
	pthread_exit(NULL);
}

void *CoordinatorThread(void *data)
{
	pthread_exit(NULL);
}

struct student {
	pthread_t thread;
	int id;
	int visits;
};

struct tutor {
	pthread_t thread;
	int id;
	bool idle;
	int currentStudent;
};

struct chair {
	int id;
	bool taken;
	int currentStudent;
};

int main(int argc, char *argv[])
{
	// Read arguments for number of threads / help
	int NUM_STUDENTS, NUM_TUTORS, NUM_CHAIRS, NUM_HELP;
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

	// Define datastructures to hold info about threads / chairs
	struct student *students = (struct student*) malloc(NUM_STUDENTS * sizeof(struct student));
	struct tutor *tutors = (struct tutor*) malloc(NUM_TUTORS * sizeof(struct tutor));
	struct tutor *chairs = (struct chair*) malloc(NUM_CHAIRS * sizeof(struct chair));

	int rc; long t;
	for(t = 0; t < NUM_STUDENTS; t++){
		// DEBUG PRINT STATEMENT
		printf("MAIN: Creating Student Thread #%ld\n", t);
		rc = pthread_create(&students[t].thread, NULL, StudentThread, (void *)t);
		if(rc) {
			//DEBUG PRINT STATEMENT
			printf("MAIN ERROR: Error Creating Thread! Code: %d\n", rc);
			exit(-1);
		}
	}
	for(t = 0; t < NUM_TUTORS; t++){
		// DEBUG PRINT STATEMENT
		printf("MAIN: Creating Tutor Thread #%ld\n", t);
		rc = pthread_create(&tutors[t].thread, NULL, StudentThread, (void *)t);
		if(rc) {
			//DEBUG PRINT STATEMENT
			printf("MAIN ERROR: Error Creating Thread! Code: %d\n", rc);
			exit(-1);
		}
	}

	pthread_exit(NULL);
}
