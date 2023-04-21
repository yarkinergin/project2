#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#define MAXTHREADS  10		// max number of threads

pthread_mutex_t lock;

struct node {
   struct burst_item data;
	
   struct node *next;
   struct node *prev;
};

struct node **heads;

struct burst_item {
   int pid;
   int burstLength;
   int arrivalTime;
   int remainingTime;
   int finishTime;
   int turnaroundTime;
   int processorId;
};

// this is the function to be executed by all the threads concurrently
static void *do_task(void *arg_ptr)
{
    pthread_mutex_lock(&lock);
    // critical section
  
    pthread_mutex_unlock(&lock);

	pthread_exit(NULL); //  tell a reason to thread waiting in join
}

int main(int argc, char *argv[])
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    int startTime = current_time.tv_usec;
    int currentTime;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    int N = 2;
    char SAP = "M";
    char QS[] = "RM";
    char ALG[] = "RR";
    int Q = 20;
    char INFILE = "in.txt";
    int OUTMODE = 1;
    char OUTFILE[] = "out.txt";
    int r[] = {200, 10, 1000, 100, 10, 500};

    if (strcmp(SAP, "M") == 0){
        heads = (struct node **)malloc(N * sizeof(struct node *));
        for (int i = 0; i < N; i++){
            heads[i] = NULL;
        }
    }
    else{
        heads = (struct node **)malloc(sizeof(struct node *));
        heads[0] = NULL;
    }

	int ret;
	pthread_t tids[MAXTHREADS];	// thread ids
	char *retmsg;

	FILE* ptr;
	char ch;

    int countPid = 0;

    for (int i = 0; i < argc; i++) {
        if (argv[i] == "-n"){
            i++;
            N = argv[i];
        }
        else if (argv[i] == "-a"){
            i++;
            SAP = argv[i];
            i++;
            strcpy(QS, argv[i]);  
        }
        else if (argv[i] == "-s"){
            i++;
            strcpy(ALG, argv[i]);
            i++;
            strcpy(Q, argv[i]);
        }
        else if (argv[i] == "-i"){
            i++;
            strcpy(INFILE, argv[i]);
        }
        else if (argv[i] == "-m"){
            i++;
            strcpy(OUTMODE, argv[i]);
        }
        else if (argv[i] == "-o"){
            i++;
            strcpy(OUTFILE, argv[i]);
        }
        else if (argv[i] == "-r"){
            for(int j = 0; j = 6; j++){
                i++;
                r[j] = argv[i];
            }
        }
    }

	ptr = fopen(INFILE, "r");
    int k = 0;
    char *textPL = "";
    char *textIAT = "";
    do {
		ch = fgetc(ptr);

        *textPL = "";
        *textIAT = "";
		if (ch == 'P'){
			k = 0;
    		ch = fgetc(ptr);
            do {
		        ch = fgetc(ptr);
                textPL[k] = ch;
                k++;
            } while (ch != EOF || ch != "\n");
            struct burst_item *burst;
            burst->pid = countPid;
            countPid++;
            burst->burstLength = atoi(textPL);
            if(strcmp(ALG, "RR") != 0)
                burst->remainingTime = atoi(textPL);
           
            gettimeofday(&current_time, NULL);
            currentTime = current_time.tv_usec;
           
            burst->arrivalTime = (currentTime - startTime);
            burst->finishTime = currentTime + burst->burstLength;
            if (strcmp(SAP, "S") == 0){
                insert(heads[0], burst);
            }
		}
        else if (ch == "A"){
            k = 0;
    		ch = fgetc(ptr);
    		ch = fgetc(ptr);
            do {
		        ch = fgetc(ptr);
                textIAT[k] = ch;
                k++;
            } while (ch != EOF || ch != "\n");
            sleep(atoi(textIAT));
        }
	} while (ch != EOF);

	for (int i = 0; i < N; ++i) {
		ret = pthread_create(&(tids[i]), NULL, do_task, (void *) argv);
       
        if (ret != 0) {
			exit(1);
		}
    }

    for (int i = 0; i < N; ++i) {
	    ret = pthread_join(tids[i], (void **)&retmsg);
		if (ret != 0) {
			exit(1);
		}
		// we got the reason as the string pointed by retmsg.
		// space for that was allocated in thread function.
        // now we are freeing the allocated space.
		free (retmsg);
	}

}

void insert(struct node* head, struct burst_item* newItem) {
   struct node *newNode = (struct node*) malloc(sizeof(struct node));
   newNode->data = *newItem;

   struct node *current = head;

   if (head != NULL){
      while( head != NULL){
         head = head->next;
         if (head == NULL)
            break;
      }
   }
   else {
      newNode->next = NULL;
      newNode->prev = NULL;
      head = newNode;
      return;
   }

   newNode->next = head;
   if (head != NULL) {
      newNode->prev = head->prev;
      head->prev = newNode;
      if (head->prev != NULL)
         head->prev->next = newNode;
   }

   head = current;
}

void deleteNode(struct node* head, struct node* del) 
{ 
    /* base case */
    if (head == NULL || del == NULL) 
        return; 
  
    /* If node to be deleted is head node */
    if (head == del) 
        head = del->next; 
  
    /* Change next only if node to be 
    deleted is NOT the last node */
    if (del->next != NULL) 
        del->next->prev = del->prev; 
  
    /* Change prev only if node to be 
    deleted is NOT the first node */
    if (del->prev != NULL) 
        del->prev->next = del->next; 
  
    /* Finally, free the memory occupied by del*/
    free(del); 
    return; 
} 