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
#include <stdbool.h>
#include <sys/syscall.h>
#include <math.h>

#define MAXTHREADS  10		// max number of threads

pthread_mutex_t lock;

struct burst_item {
   int pid;
   int burstLength;
   int arrivalTime;
   int remainingTime;
   int finishTime;
   int turnaroundTime;
   int waitingTime;
   int processorId;
};

struct node {
   struct burst_item data;
	
   struct node *next;
   struct node *prev;
};

struct node **heads;
struct node *list;
int *headsLengths;
pthread_t tids[MAXTHREADS];	// thread ids
int startTime;
struct timeval current_time;
int sizeArgv;

void insert(struct node* head, struct burst_item* newItem);
void deleteNode(struct node* head, struct node* del);

// this is the function to be executed by all the threads concurrently
static void *do_task(void *arg_ptr)
{
    char SAP = 'M';
    char QS[] = "RM";
    char ALG[] = "RR";
    int Q = 20;
    char INFILE[] = "in.txt";
    int OUTMODE = 1;
    char OUTFILE[] = "out.txt";

    char **argv = (char **)arg_ptr;   

    for (int i = 0; i < sizeArgv; i++) {
        if (strcmp(argv[i], "-a") == 0){
            i++;
            SAP = argv[i][0];
            i++;
            strcpy(QS, argv[i]);  
        }
        else if (strcmp(argv[i], "-s") == 0){
            i++;
            strcpy(ALG, argv[i]);
            i++;
            Q = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-i") == 0){
            i++;
            strcpy(INFILE, argv[i]);
        }
        else if (strcmp(argv[i], "-m") == 0){
            i++;
            OUTMODE = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-o") == 0){
            i++;
            strcpy(OUTFILE, argv[i]);
        }
    }
    if (strcmp(ALG, "RR") != 0) {
        Q = 0;
    }

    int queueId;
    pid_t tid = syscall(__NR_gettid);

    while( heads[tid]->data.pid != -1){
        pthread_mutex_lock(&lock);
        // critical section
        
        if (SAP == 'S'){
            queueId = 0;
        }
        else {
            queueId = tid;
        }

        while(heads[queueId] == NULL)
            sleep(1);

        struct burst_item *burst;

        if (strcmp(ALG, "FCFS") == 0){
            burst = &heads[queueId]->data;

            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainintime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            sleep(burst->burstLength);
            
            gettimeofday(&current_time, NULL);
            int currentTime = current_time.tv_usec;

            burst->finishTime = currentTime - startTime;
            burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
            burst->waitingTime = burst->turnaroundTime - burst->burstLength;

            insert(list, burst);
            deleteNode(heads[queueId], heads[queueId]);
        }
        else if(strcmp(ALG, "SJF") == 0){
            struct node *curNode = heads[queueId];
            int sjLength = curNode->data.burstLength;
            int sjIndex = 0;
            int count = 0;

            while(curNode != NULL){
                if (curNode->data.burstLength < sjLength) {
                    sjLength = curNode->data.burstLength;
                    sjIndex = count;
                }
                curNode = curNode->next;
                count++;
            }

            curNode = heads[queueId];
            count = 0;
            
            while (count != sjIndex){
                curNode = curNode->next;
                count++;
            }

            burst = &curNode->data;

            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainintime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            sleep(burst->burstLength);

            gettimeofday(&current_time, NULL);
            int currentTime = current_time.tv_usec;

            burst->finishTime = currentTime - startTime;
            burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
            burst->waitingTime = burst->turnaroundTime - burst->burstLength;

            insert(list, burst);
            deleteNode(heads[queueId], curNode);
        }
        else if(strcmp(ALG, "RR") == 0){
            burst = &heads[queueId]->data;

            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainintime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            if (burst->remainingTime > Q){                
                sleep(Q);

                burst->remainingTime = burst->remainingTime - Q;
                deleteNode(heads[queueId], heads[queueId]);
                insert(heads[queueId], burst);
            }
            else {
                sleep(burst->remainingTime);

                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec;

                burst->finishTime = currentTime - startTime;
                burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
                burst->waitingTime = burst->turnaroundTime - burst->burstLength;

                insert(list, burst);
                deleteNode(heads[queueId], heads[queueId]);
            }
        }
    
        pthread_mutex_unlock(&lock);
    }

	pthread_exit(NULL); //  tell a reason to thread waiting in join
}

int main(int argc, char *argv[])
{
    gettimeofday(&current_time, NULL);
    startTime = current_time.tv_usec;
    int currentTime;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    sizeArgv = argc;

    int N = 2;
    char SAP = 'M';
    char QS[] = "RM";
    char ALG[] = "RR";
    int Q = 20;
    char INFILE[] = "in.txt";
    char OUTFILE[] = "out.txt";
    int randS[] = {200, 10, 1000, 100, 10, 500};
    
    bool infileMode = false;

    if (SAP =='M'){
        heads = (struct node **)malloc(N * sizeof(struct node *));
        for (int i = 0; i < N; i++){
            heads[i] = NULL;
        }
    }
    else{
        heads = (struct node **)malloc(sizeof(struct node *));
        heads[0] = NULL;
    }
    list = NULL;

    headsLengths = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++){
        headsLengths[i] = 0;
    }

	int ret;
	char *retmsg;

	FILE* ptr;
	char ch;

    int countPid = 0;
    int countRR = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0){
            i++;
            N = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-a") == 0){
            i++;
            SAP = argv[i][0];
            i++;
            strcpy(QS, argv[i]);  
        }
        else if (strcmp(argv[i], "-s") == 0){
            i++;
            strcpy(ALG, argv[i]);
            i++;
            Q = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-i") == 0){
            i++;
            strcpy(INFILE, argv[i]);
        }
        else if (strcmp(argv[i], "-o") == 0){
            i++;
            strcpy(OUTFILE, argv[i]);
        }
        else if (strcmp(argv[i], "-r") == 0){
            for(int j = 0; j < 6; j++){
                i++;
                randS[j] = atoi(argv[i]);
            }
            infileMode = false;
        }
    }

    if (strcmp(ALG, "RR") != 0) {
        Q = 0;
    }

    for (int i = 0; i < N; ++i) {
		ret = pthread_create(&(tids[i]), NULL, do_task, (void *) argv);
       
        if (ret != 0) {
			exit(1);
		}
    }

    if(infileMode){
        ptr = fopen(INFILE, "r");
        int k = 0;
        char *textPL = "";
        char *textIAT = "";
        do {
            ch = fgetc(ptr);

            textPL = "";
            textIAT = "";
            if (ch == 'P'){
                k = 0;
                ch = fgetc(ptr);
                do {
                    ch = fgetc(ptr);
                    textPL[k] = ch;
                    k++;
                } while (ch != EOF || ch != '\n');
                struct burst_item *burst;
                burst->pid = countPid;
                countPid++;
                burst->burstLength = atoi(textPL);
                burst->remainingTime = atoi(textPL);
            
                gettimeofday(&current_time, NULL);
                currentTime = current_time.tv_usec;
            
                burst->arrivalTime = (currentTime - startTime);
                if (SAP == 'S'){
                    burst->processorId = 1;
                    insert(heads[0], burst);
                    headsLengths[0] = headsLengths[0] + 1;
                }
                else {
                    if (strcmp(QS, "RM")){
                        burst->processorId = (countRR % N) + 1;
                        insert(heads[countRR % N], burst);
                        countRR++;
                    }
                    else {
                        int minIndex = 0;
                        for(int x = 0; x < N; x++){
                            if(headsLengths[x] < headsLengths[minIndex])
                                minIndex = x;
                        }
                        burst->processorId = minIndex + 1;
                        
                        insert(heads[minIndex], burst);
                        headsLengths[minIndex] = headsLengths[minIndex] + 1;
                    }
                }
            }
            else if (ch == 'A'){
                k = 0;
                ch = fgetc(ptr);
                ch = fgetc(ptr);
                do {
                    ch = fgetc(ptr);
                    textIAT[k] = ch;
                    k++;
                } while (ch != EOF || ch != '\n');
                sleep(atoi(textIAT));
            }
        } while (ch != EOF);
    }
    else{
        int count = 0;

        while(count < 6){
            double lambdaT = 1 / randS[0];
            double uT = ((double)rand()) / RAND_MAX;
            double xT = ((-1) * log(1 - uT)) / lambdaT;

            double lambdaL = 1 / randS[3];
            double uL = ((double)rand()) / RAND_MAX;
            double xL = ((-1) * log(1 - uL)) / lambdaL;

            if (xT > randS[1] && xT < randS[2] && xL > randS[4] && xL < randS[5])
            {
                struct burst_item *burst;
                burst->pid = countPid;
                countPid++;

                burst->burstLength = xL;
                burst->remainingTime = xL;

                gettimeofday(&current_time, NULL);
                currentTime = current_time.tv_usec;

                burst->arrivalTime = currentTime - startTime;

                if (SAP == 'S'){
                    burst->processorId = 1;
                    insert(heads[0], burst);
                    headsLengths[0] = headsLengths[0] + 1;
                }
                else {
                    if (strcmp(QS, "RM")){
                        burst->processorId = (countRR % N) + 1;
                        insert(heads[countRR % N], burst);
                        countRR++;
                    }
                    else {
                        int minIndex = 0;
                        for(int x = 0; x < N; x++){
                            if(headsLengths[x] < headsLengths[minIndex])
                                minIndex = x;
                        }
                        burst->processorId = minIndex + 1;
                        
                        insert(heads[minIndex], burst);
                        headsLengths[minIndex] = headsLengths[minIndex] + 1;
                    }
                }
                sleep(xT);
            }
            
        }
    }
    
    if (SAP == 'S'){
        struct node *dummyItem = (struct node*) malloc(sizeof(struct node));
        (&dummyItem->data)->pid = -1;
        struct node *currentNode;

        currentNode = heads[0];
        while( currentNode != NULL){
            currentNode = currentNode->next;
        }

        currentNode->prev->next = dummyItem;
        dummyItem->prev = currentNode->prev;
        dummyItem->next = currentNode->next;
    }
    else{
        for (int i = 0; i < N; i++)
        {
            struct node *dummyItem = (struct node*) malloc(sizeof(struct node));
            (&dummyItem->data)->pid = -1;
            struct node *currentNode;

            currentNode = heads[0];
            while( currentNode != NULL){
                currentNode = currentNode->next;
            }

            currentNode->prev->next = dummyItem;
            dummyItem->prev = currentNode->prev;
            dummyItem->next = currentNode->next;
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

    printf("pid\tcpu\tburstlen\tarv\tfinish\twaitingtime\tturnaround\n");
    while(list != NULL) {
        struct node *curNode = list;
        int minPid = list->data.pid;
        int minIndex = 0;
        int count = 0;
        struct burst_item burst;

        while(curNode != NULL){
            if(curNode->data.pid < minPid){
                minPid = curNode->data.pid;
                minIndex = count;
            }
            curNode = curNode->next;
        }
        curNode = 0;
        count = 0;

        while(count < minIndex){
            curNode = curNode->next;
            count++;
        }
        burst = curNode->data;

        printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", burst.pid, burst.processorId, burst.burstLength, burst.arrivalTime, burst.finishTime, burst.waitingTime, burst.turnaroundTime);
        deleteNode(list,curNode);
    }

}

void insert(struct node* head, struct burst_item* newItem) {
   struct node *newNode = (struct node*) malloc(sizeof(struct node));
   newNode->data = *newItem;

   struct node *current = head;

   if (head != NULL){
      while( head != NULL || head->data.pid != -1){
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