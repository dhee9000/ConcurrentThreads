// Compile using:  gcc csmc.c -Wall -pthread -lpthread -std=c99 -o CSMC
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_SLEEP 2
#define TUTOR_TIME 0.2

typedef struct {
    int id;
    pthread_t thread;
    int visits;
    sem_t notifyStudent;
} Student;

typedef struct {
    int id;
    pthread_t thread;
} Tutor;

typedef struct {
    bool taken;
    int studentId;
    int studentVisits;
    int arrivedAt;
    int priority;
    sem_t *notifyStudent;
    int tutoredBy;
} Chair;

Chair *chairs;
int chairsTaken = 0, arrivedStudentId = -1, arrivedStudentVisits = -1;
sem_t *arrivedStudentNotifier;

sem_t tutorNeeded;
sem_t room_mutex;
sem_t chairs_mutex;
sem_t notifyCoordinator;

int coordinatorRequests = 0, tutoringCompleted = 0, activeTutoring = 0;

int NUM_CHAIRS = 0, NUM_HELP = 0;

void *StudentThread(void *data)
{
    // Get student record
    Student self = *(Student*)data;
    
    while(self.visits < NUM_HELP){

        // Program for some time
        sleep(rand()%MAX_SLEEP);

        // Try and get a tutor
        // Wait for access to check chairs and notify coordinator
        sem_wait(&room_mutex);
        if(chairsTaken < NUM_CHAIRS){
            // Take a chair, chair info will be filled in by coordinator
            chairsTaken++;
            printf("St: Student %d takes a seat. Empty chairs = %d.\n", self.id, NUM_CHAIRS - chairsTaken);
            
            // Notify coordinator, release chairs mutex
            arrivedStudentId = self.id; arrivedStudentVisits = self.visits; arrivedStudentNotifier = &self.notifyStudent;
            sem_post(&notifyCoordinator);
            sem_post(&room_mutex);
            
            // Wait until notified by tutor
            sem_wait(&self.notifyStudent);

            // Get chair info
            int chairIndex = 0;
            for(int i = 0; i < NUM_CHAIRS; i++){
                if(chairs[i].taken && (chairs[i].studentId == self.id))
                    chairIndex = i; break;
            }
            if(chairIndex == -1)
                exit(-1);
            int tutoredBy = chairs[chairIndex].tutoredBy;

            // Get tutored for TUTOR_TIME and then increment visits
            sleep(TUTOR_TIME);
            printf("St: Student %d received help from Tutor %d.\n", self.id, tutoredBy);
            chairs[chairIndex].taken = false;
            chairsTaken--;
            self.visits++;
        }
        else{
            // No empty chairs found, continue programming.
            printf("St: Student %d found no empty chair. Will try again later\n", self.id);
            sem_post(&room_mutex);
            continue;
        }
    }

	pthread_exit(NULL);
}

void *TutorThread(void *data)
{
    // Get tutor record
    Tutor self = *(Tutor*)data;
    
    while(1){
        // Wait until tutor needed
        sem_wait(&tutorNeeded);

        // Get highest priority student
        int highestPriority = 0;
        for(int i = 1; i < NUM_CHAIRS; i++){
            if(chairs[i].priority > chairs[highestPriority].priority)
                highestPriority = i;
        }

        // Notify the student that tutor is ready
        chairs[highestPriority].tutoredBy = self.id;
        activeTutoring++;
        sem_post(chairs[highestPriority].notifyStudent);

        // Tutor the student
        sleep(TUTOR_TIME);
		tutoringCompleted++;
        printf("Tu: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n", chairs[highestPriority].studentId, self.id, activeTutoring, tutoringCompleted);
        activeTutoring--;
    }
}

void *CoordinatorThread(void *data)
{
    while(1){
        // Wait until the coordinator is notified
        sem_wait(&notifyCoordinator);

        // Get the Id of the notifying student
        int studentId = arrivedStudentId; arrivedStudentId = -1;
        int studentVisits = arrivedStudentVisits; arrivedStudentVisits = -1;
		sem_t * studentNotifier = arrivedStudentNotifier; arrivedStudentNotifier = NULL;
        coordinatorRequests++;

        // Find the first empty chair
        int currentChair = -1;
        for(int i = 0; i < NUM_CHAIRS; i++){
            if(!chairs[i].taken)
                currentChair = i; break;
        }
        if(currentChair == -1)
            exit(-1);

        // Place the student there
        chairs[currentChair].taken = true;
        chairs[currentChair].studentId = studentId;
        chairs[currentChair].arrivedAt = coordinatorRequests;
        chairs[currentChair].studentVisits = studentVisits;
		chairs[currentChair].notifyStudent = studentNotifier;

        // Calculate and update chair priority
        int priority = -1, priorityLower = 0, priorityHigher = 0;
        for(int i = 0; i < NUM_CHAIRS; i++){
            if(i == currentChair)
                continue;
            if(chairs[i].taken){
                if((chairs[i].studentVisits < studentVisits) || ((chairs[i].studentVisits == studentVisits) && (chairs[i].arrivedAt < chairs[currentChair].arrivedAt))){
                    priorityHigher++;
                }
                else if((chairs[i].studentVisits > studentVisits) || ((chairs[i].studentVisits == studentVisits) && (chairs[i].arrivedAt > chairs[currentChair].arrivedAt))){
                    priorityLower++;
                    chairs[i].priority++;
                }
            }
            else{
                continue;
            }
        }
        priority = priorityLower;
        chairs[currentChair].priority = priority;
        printf("Co: Student %d with priority %d in the queue. Waiting students now = %d. Total requests = %d\n", studentId, priority, chairsTaken, coordinatorRequests);

		// Notify the tutors
		sem_post(&tutorNeeded);
    }
}

int main(int argc, char *argv[])
{
	// Read arguments for number of threads / help
	int NUM_STUDENTS, NUM_TUTORS;
	if(argc ==  5){
		NUM_STUDENTS = atoi(argv[1]);
		NUM_TUTORS = atoi(argv[2]);
		NUM_CHAIRS = atoi(argv[3]);
		NUM_HELP = atoi(argv[4]);
	}
	else {
		printf("ERROR! Not Enough Arguments! Syntax: CSMC #students #tutors #chairs #help\n");
		exit(-1);
	}

	// Initialize semaphores
    sem_init(&room_mutex, 0, 1);
    sem_init(&notifyCoordinator, 0, 0);
    sem_init(&tutorNeeded, 0, 0);

	// Define datastructures to hold info about threads / chairs
	Student *students = (Student*) malloc(NUM_STUDENTS * sizeof(Student));
	Tutor *tutors = (Tutor*) malloc(NUM_TUTORS * sizeof(Tutor));
    chairs = (Chair*) malloc(NUM_CHAIRS * sizeof(Chair));

    // Create coordinator thread
	pthread_t coordinator;
	pthread_create(&coordinator, NULL, CoordinatorThread, (void*)&NUM_CHAIRS);

    // Create threads for students and tutors
	int rc; long t;
	for(t = 0; t < NUM_STUDENTS; t++){
		students[t].id = t;
        students[t].visits = 0;
        sem_init(&students[t].notifyStudent, 0, 0);
        rc = pthread_create(&students[t].thread, NULL, StudentThread, (void *)&students[t]);
		if(rc) {
			exit(-1);
		}
	}
	for(t = 0; t < NUM_TUTORS; t++){
		tutors[t].id = t;
		rc = pthread_create(&tutors[t].thread, NULL, TutorThread, (void *)&tutors[t]);
		if(rc) {
			exit(-1);
		}
	}

    // Wait for all students to finish
	for(t = 0; t < NUM_STUDENTS; t++){
		pthread_join(students[t].thread, NULL);
	}

	printf("Exiting Main Thread!\n");
	return 0;
}