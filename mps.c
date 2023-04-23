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

struct arg {
    char **argv;
    int t_index;
};

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

pthread_mutex_t lock;
struct node **heads;
struct node *list;
int *headsLengths;
pthread_t tids[MAXTHREADS];	// thread ids
int startTime;
struct timeval current_time;
int sizeArgv;

void insert(struct node**, struct burst_item*);
void deleteNode(struct node**, struct node*);

// this is the function to be executed by all the threads concurrently
static void *do_task(void *arg_ptr)
{
    printf("*************************\n");

    char SAP = 'M';
    char QS[2] = "RM";
    char ALGT[5] = "RR";
    int Q = 20;
    char INFILE[10] = "in.txt";
    int OUTMODE = 1;
    char OUTFILE[10] = "out.txt";
    int queueId;

    char **argv = ((struct arg *) arg_ptr)->argv;
    int tid = ((struct arg *) arg_ptr)->t_index;

    for (int i = 0; i < sizeArgv; i++) {
        if (strcmp(argv[i], "-a") == 0){
            i++;
            SAP = argv[i][0];
            i++;
            strcpy(QS, argv[i]); 
        }
        else if (strcmp(argv[i], "-s") == 0){
            i++;
            strcpy(ALGT, argv[i]);
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

    if (SAP == 'S'){
        queueId = 0;
    }
    else {
        queueId = tid - 1;
    }

    while(heads[queueId] == NULL){
        usleep(1);
    }

    while( heads[queueId]->data.pid > 0){
        pthread_mutex_lock(&lock);
        // critical section

        struct burst_item *burst;

        if (strcmp(ALGT, "FCFS") == 0 && heads[queueId]->data.pid > 0){
            burst = &heads[queueId]->data;

            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainintime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            usleep(burst->burstLength);
            
            gettimeofday(&current_time, NULL);
            int currentTime = current_time.tv_usec;

            burst->finishTime = currentTime - startTime;
            burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
            burst->waitingTime = burst->turnaroundTime - burst->burstLength;

            insert(&list, burst);
            deleteNode(&heads[queueId], heads[queueId]);
            headsLengths[queueId]--;

            /*
            printf("-----------------\n");
            struct node *cur = heads[0];
            while(cur != NULL){
                printf("%d\n", cur->data.pid);
                cur = cur->next;
            }
            */
        }
        else if(strcmp(ALGT, "SJF") == 0){
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

            usleep(burst->burstLength);

            gettimeofday(&current_time, NULL);
            int currentTime = current_time.tv_usec;

            burst->finishTime = currentTime - startTime;
            burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
            burst->waitingTime = burst->turnaroundTime - burst->burstLength;

            insert(&list, burst);
            deleteNode(&heads[queueId], curNode);
        }
        else if(strcmp(ALGT, "RR") == 0){
            burst = &heads[queueId]->data;

            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainintime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            if (burst->remainingTime > Q){                
                usleep(Q);

                burst->remainingTime = burst->remainingTime - Q;
                deleteNode(&heads[queueId], heads[queueId]);
                insert(&heads[queueId], burst);
            }
            else {
                usleep(burst->remainingTime);

                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec;

                burst->finishTime = currentTime - startTime;
                burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
                burst->waitingTime = burst->turnaroundTime - burst->burstLength;

                insert(&list, burst);
                deleteNode(&heads[queueId], heads[queueId]);
            }
        }
    
        pthread_mutex_unlock(&lock);
    }

	pthread_exit(NULL); //  tell a reason to thread waiting in join
}

int main(int argc, char *argv[])
{
    printf("---------------------------\n");

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
    char QS[2] = "RM";
    char ALG[5] = "RR";
    char INFILE[10] = "in.txt";
    char OUTFILE[10] = "out.txt";
    int randS[6] = {200, 10, 1000, 100, 10, 500};
    
    bool infileMode = false;
    list = NULL;
	struct arg t_args[MAXTHREADS];	// thread function arguments

    headsLengths = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++){
        headsLengths[i] = 0;
    }

	int ret;
	char *retmsg;

	FILE* ptr;
	char ch;

    int countPid = 1;
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
        }
        else if (strcmp(argv[i], "-i") == 0){
            i++;
            strcpy(INFILE, argv[i]);
            infileMode = true;
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

    pthread_mutex_lock(&lock);

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

    if(infileMode){
        ptr = fopen(INFILE, "r");
        int k = 0;
        char textPL[4];
        char textIAT[4];
        do {
            ch = fgetc(ptr);

            if (ch == 'P'){            
                k = 0;
                strcpy(textPL, "");
                ch = fgetc(ptr);
                ch = fgetc(ptr);
                do {
                    textPL[k] = ch;
                    ch = fgetc(ptr);
                    k++;
                } while (ch != EOF && ch != '\n' && ch != ' ');
                struct burst_item *burst = (struct burst_item*) malloc(sizeof(struct burst_item));
                burst->pid = countPid;
                countPid++;
                burst->burstLength = atoi(textPL);
                burst->remainingTime = atoi(textPL);
            
                gettimeofday(&current_time, NULL);
                currentTime = current_time.tv_usec;
            
                burst->arrivalTime = (currentTime - startTime);
                if (SAP == 'S'){
                    burst->processorId = 1;
                    insert(&heads[0], burst);
                    headsLengths[0] = headsLengths[0] + 1;
                }
                else {
                    if (strcmp(QS, "RM")){
                        burst->processorId = (countRR % N) + 1;
                        insert(&heads[countRR % N], burst);
                        countRR++;
                    }
                    else {
                        int minIndex = 0;
                        for(int x = 0; x < N; x++){
                            if(headsLengths[x] < headsLengths[minIndex])
                                minIndex = x;
                        }
                        burst->processorId = minIndex + 1;
                        
                        insert(&heads[minIndex], burst);
                        headsLengths[minIndex] = headsLengths[minIndex] + 1;
                    }
                }
            }
            else if (ch == 'I'){
                k = 0;
                strcpy(textIAT, "");
                ch = fgetc(ptr);
                ch = fgetc(ptr);
                ch = fgetc(ptr);
                do {
                    textIAT[k] = ch;
                    k++;
                    ch = fgetc(ptr);
                } while (ch != EOF && ch != '\n');
                usleep(atoi(textIAT));
            }
        } while (ch != EOF);
    }
    else{
        int count = 0;

        while(count < 6){
            double lambdaT = 1.0 / randS[0];
            double uT = ((double)rand()) / RAND_MAX;
            double xT = ((-1) * log(1 - uT)) / lambdaT;

            double lambdaL = 1.0 / randS[3];
            double uL = ((double)rand()) / RAND_MAX;
            double xL = ((-1) * log(1 - uL)) / lambdaL;

            if (xT > randS[1] && xT < randS[2] && xL > randS[4] && xL < randS[5])
            {
                count++;
                struct burst_item *burst = (struct burst_item*) malloc(sizeof(struct burst_item));
                burst->pid = countPid;
                countPid++;

                burst->burstLength = xL;
                burst->remainingTime = xL;

                gettimeofday(&current_time, NULL);
                currentTime = current_time.tv_usec;

                burst->arrivalTime = currentTime - startTime;

                if (SAP == 'S'){
                    burst->processorId = 1;
                    insert(&heads[0], burst);
                    headsLengths[0] = headsLengths[0] + 1;
                }
                else {
                    if (strcmp(QS, "RM")){
                        burst->processorId = (countRR % N) + 1;
                        insert(&heads[countRR % N], burst);
                        countRR++;
                    }
                    else {
                        int minIndex = 0;
                        for(int x = 0; x < N; x++){
                            if(headsLengths[x] < headsLengths[minIndex])
                                minIndex = x;
                        }
                        burst->processorId = minIndex + 1;
                        
                        insert(&heads[minIndex], burst);
                        headsLengths[minIndex] = headsLengths[minIndex] + 1;
                    }
                }
                usleep(xT);
            }
        }
    }
    
    if (SAP == 'S'){
        struct node *dummyItem = (struct node*) malloc(sizeof(struct node));
        (&dummyItem->data)->pid = -1;
        struct node *currentNode;
        struct node *prevNode;

        currentNode = heads[0];
        while( currentNode != NULL){
            prevNode = currentNode;
            currentNode = currentNode->next;
        }

        if(prevNode != NULL){
            prevNode->next = dummyItem;
            dummyItem->prev = prevNode;
            dummyItem->next = NULL;
        }
        else{
            dummyItem->next = NULL;
            dummyItem->prev = NULL;
        }
    }
    else{
        for (int i = 0; i < N; i++)
        {
            struct node *dummyItem = (struct node*) malloc(sizeof(struct node));
            (&dummyItem->data)->pid = -1;
            struct node *currentNode;
            struct node *prevNode;

            currentNode = heads[0];
            while( currentNode != NULL){
                prevNode = currentNode;
                currentNode = currentNode->next;
            }

            if(prevNode != NULL){
                prevNode->next = dummyItem;
                dummyItem->prev = prevNode;
                dummyItem->next = NULL;
            }
            else{
                dummyItem->next = NULL;
                dummyItem->prev = NULL;
            }
        }
    }

    struct node *cur = heads[0];
    while(cur != NULL){
        printf("%d\t%d\n", cur->data.pid, cur->data.burstLength);
        cur = cur->next;
    }

    pthread_mutex_unlock(&lock);

    for (int i = 0; i < N; ++i) {
        t_args[i].argv = argv;
        t_args[i].t_index = i + 1;
		
        ret = pthread_create(&(tids[i]), NULL, do_task, (void *) &(t_args[i]));
       
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
        curNode = list;

        while(count < minIndex){
            curNode = curNode->next;
            count++;
        }
        burst = curNode->data;

        printf("%d\t%d\t%d\t\t%d\t%d\t%d\t\t%d\n", burst.pid, burst.processorId, burst.burstLength, burst.arrivalTime, burst.finishTime, burst.waitingTime, burst.turnaroundTime);
        deleteNode(&list,curNode);
    }
}

void insert(struct node** head, struct burst_item* newItem) {
    struct node *newNode = (struct node*) malloc(sizeof(struct node));
    newNode->data = *newItem;

    struct node *current = *head;
    struct node *prev = current;

    if (*head != NULL){
        while( *head != NULL && (*head)->data.pid != -1){
            prev = *head;
            *head = (*head)->next;            
        }
    }
    else {
        newNode->next = NULL;
        newNode->prev = NULL;
        *head = newNode;
        return;
    }

    newNode->next = *head;
    newNode->prev = prev;
    if (prev != NULL) {
        prev->next = newNode;
    }
    if (*head != NULL) {
        (*head)->prev = newNode;
    }

    *head = current;
}

void deleteNode(struct node** head_ref, struct node* del)
{
    /* base case */
    if (*head_ref == NULL || del == NULL)
        return;
  
    /* If node to be deleted is head node */
    if (*head_ref == del)
        *head_ref = del->next;
  
    /* Change next only if node to be deleted is NOT the last node */
    if (del->next != NULL)
        del->next->prev = del->prev;
  
    /* Change prev only if node to be deleted is NOT the first node */
    if (del->prev != NULL)
        del->prev->next = del->next;
  
    /* Finally, free the memory occupied by del*/
    free(del);
    return;
}