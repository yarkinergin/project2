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

pthread_mutex_t* locks;

pthread_cond_t* cvs;

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
    char SAP = 'M';
    char QS[3] = "RM";
    char ALGT[5] = "RR";
    int Q = 20;
    int OUTMODE = 1;
    char OUTFILE[10] = "out.txt";
    int queueId;

    FILE *fptr;

    char **argv = ((struct arg *) arg_ptr)->argv;
    int tid = ((struct arg *) arg_ptr)->t_index;
    pthread_mutex_t *lock;
    pthread_cond_t* cv;

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
        else if (strcmp(argv[i], "-m") == 0){
            i++;
            OUTMODE = OUTMODE + atoi(argv[i]) - 1;
        }
        else if (strcmp(argv[i], "-o") == 0){
            i++;
            strcpy(OUTFILE, argv[i]);
            *(&OUTMODE) = *(&OUTMODE) + 3;
        }
    }

    if (OUTMODE > 3)
        fptr = fopen(OUTFILE,"a");

    if (SAP == 'S'){
        queueId = 0;
    }
    else {
        queueId = tid - 1;
    }

    lock = &locks[queueId];
    cv = &cvs[queueId];

    while( 1){
        pthread_mutex_lock(lock);
        // critical section
        while(heads[queueId] == NULL){
            pthread_cond_wait(cv, lock);
        }

        if(heads[queueId]->data.pid < 0){
            pthread_cond_signal(cv);
            pthread_mutex_unlock(lock);
            break;
        }

        struct burst_item *burst;

        if (strcmp(ALGT, "FCFS") == 0 && heads[queueId]->data.pid > 0){
            burst = &heads[queueId]->data;

            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 3) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("Burst started: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 5) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 6) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "Burst started: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            usleep(burst->burstLength);

            if (OUTMODE == 3) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("Burst finished: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 6) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "Burst finished: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            
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
        else if(strcmp(ALGT, "SJF") == 0 && heads[queueId]->data.pid > 0){
            struct node *curNode = heads[queueId];
            int sjLength = curNode->data.burstLength;
            int sjIndex = 0;
            int count = 0;

            while(curNode != NULL){
                if (curNode->data.burstLength < sjLength && curNode->data.burstLength > 0) {
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
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 3) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("Burst started: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 5) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 6) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "Burst started: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            usleep(burst->burstLength);

            if (OUTMODE == 3) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("Burst finished: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            if (OUTMODE == 6) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "Burst finished: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            gettimeofday(&current_time, NULL);
            int currentTime = current_time.tv_usec;

            burst->finishTime = currentTime - startTime;
            burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
            burst->waitingTime = burst->turnaroundTime - burst->burstLength;

            insert(&list, burst);
            deleteNode(&heads[queueId], curNode);
            headsLengths[queueId]--;
        }
        else if(strcmp(ALGT, "RR") == 0 && heads[queueId]->data.pid > 0){            
            burst = &heads[queueId]->data;
            
            if (OUTMODE == 2) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 3) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                printf("Burst started: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 5) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }
            else if (OUTMODE == 6) {
                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec - startTime;
                fprintf(fptr, "Burst started: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
            }

            if (burst->remainingTime > Q){    
                struct burst_item *burst1 = (struct burst_item*) malloc(sizeof(struct burst_item));

                usleep(Q);

                burst1->pid = burst->pid;
                burst1->burstLength = burst->burstLength;
                burst1->arrivalTime = burst->arrivalTime;
                burst1->remainingTime = burst->remainingTime - Q;
                burst1->processorId = burst->processorId;

                deleteNode(&heads[queueId], heads[queueId]);
                insert(&heads[queueId], burst1);
            }
            else {
                usleep(burst->remainingTime);

                gettimeofday(&current_time, NULL);
                int currentTime = current_time.tv_usec;

                burst->remainingTime = 0;
                burst->finishTime = currentTime - startTime;
                burst->turnaroundTime = burst->finishTime - burst->arrivalTime;
                burst->waitingTime = burst->turnaroundTime - burst->burstLength;

                if (OUTMODE == 3) {
                    gettimeofday(&current_time, NULL);
                    int currentTime = current_time.tv_usec - startTime;
                    printf("Burst finished: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
                }
                else if (OUTMODE == 6) {
                    gettimeofday(&current_time, NULL);
                    int currentTime = current_time.tv_usec - startTime;
                    fprintf(fptr, "Burst finished: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, tid, burst->pid, burst->burstLength, burst->remainingTime);
                }

                insert(&list, burst);
                deleteNode(&heads[queueId], heads[queueId]);
                headsLengths[queueId]--;
            }
        }
        pthread_cond_signal(cv);
        pthread_mutex_unlock(lock);
    }

    if (OUTMODE > 3){
        fclose(fptr);
    }
	pthread_exit(NULL); //  tell a reason to thread waiting in join
}

int main(int argc, char *argv[])
{
    gettimeofday(&current_time, NULL);
    startTime = current_time.tv_usec;
    int currentTime;

    sizeArgv = argc;

    int N = 2;
    char SAP = 'M';
    char QS[4] = "RM";
    char INFILE[10] = "in.txt";
    char OUTFILE[10] = "out.txt";
    int OUTMODE = 1;
    int randS[6] = {200, 10, 1000, 100, 10, 500};
    int PC = 10;

    bool infileMode = false;
    list = NULL;
	struct arg t_args[MAXTHREADS];	// thread function arguments

    headsLengths = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++){
        headsLengths[i] = 0;
    }

	int ret;
	char *retmsg;

    FILE *fptr;
	FILE* ptr;
	char ch;
    char textPL[8];
    char textIAT[8];

    int countPid = 1;
    int countRR = 0;

    int totalTA = 0;
    int countTA = 0;

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
        else if (strcmp(argv[i], "-i") == 0){
            i++;
            strcpy(INFILE, argv[i]);
            infileMode = true;
        }
        else if (strcmp(argv[i], "-m") == 0){
            i++;
            OUTMODE = OUTMODE + atoi(argv[i]) - 1;
        }
        else if (strcmp(argv[i], "-o") == 0){
            i++;
            strcpy(OUTFILE, argv[i]);
            OUTMODE = OUTMODE + 3;
        }
        else if (strcmp(argv[i], "-r") == 0){
            for(int j = 0; j < 6; j++){
                i++;
                randS[j] = atoi(argv[i]);
            }
            i++;
            PC = atoi(argv[i]);
            infileMode = false;
        }
    }

    if (SAP =='M'){
        heads = (struct node **)malloc(N * sizeof(struct node *));
        locks = (pthread_mutex_t *)malloc(N * sizeof(pthread_mutex_t));
        cvs = (pthread_cond_t *)malloc(N * sizeof(pthread_cond_t));
        for (int i = 0; i < N; i++){
            heads[i] = NULL;
            if (pthread_mutex_init(&locks[i], NULL) != 0) {
                printf("\n mutex init has failed\n");
                return 1;
            }
            if (pthread_cond_init(&cvs[i], NULL) != 0) {
                printf("\n cond init has failed\n");
                return 1;
            }
        }
    }
    else{
        heads = (struct node **)malloc(sizeof(struct node *));
        locks = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        cvs = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
        heads[0] = NULL;
        if (pthread_mutex_init(&locks[0], NULL) != 0) {
            printf("\n mutex init has failed\n");
            return 1;
        }
        if (pthread_cond_init(&cvs[0], NULL) != 0) {
            printf("\n cond init has failed\n");
            return 1;
        }
    }

    for (int i = 0; i < N; ++i) {
        t_args[i].argv = argv;
        t_args[i].t_index = i + 1;
        
        ret = pthread_create(&(tids[i]), NULL, do_task, (void *) &(t_args[i]));
       
        if (ret != 0) {
			exit(1);
		}
    }

    if(infileMode){
        ptr = fopen(INFILE, "r");
        fptr = fopen(OUTFILE,"w");
        int k = 0;
        
        do {
            ch = fgetc(ptr);

            if (ch == 'P'){            
                k = 0;
                memset(textPL,0,strlen(textPL));
                ch = fgetc(ptr);
                ch = fgetc(ptr);
                ch = fgetc(ptr);
                do {
                    textPL[k] = ch;

                    ch = fgetc(ptr);
                    k++;
                } while (ch != EOF && isdigit(ch));
                struct burst_item *burst = (struct burst_item*) malloc(sizeof(struct burst_item));
                burst->pid = countPid;
                countPid++;
                burst->burstLength = atoi(textPL);
                burst->remainingTime = atoi(textPL);
            
                gettimeofday(&current_time, NULL);
                currentTime = current_time.tv_usec;
            
                burst->arrivalTime = (currentTime - startTime);
                if (SAP == 'S'){
                    pthread_mutex_lock(&locks[0]);
                    
                    burst->processorId = 1;

                    insert(&heads[0], burst);

                    headsLengths[0] = headsLengths[0] + 1;

                    if (OUTMODE == 3) {
                        gettimeofday(&current_time, NULL);
                        int currentTime = current_time.tv_usec - startTime;
                        printf("Burst added queue: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, burst->processorId, burst->pid, burst->burstLength, burst->remainingTime);
                    }
                    else if (OUTMODE == 6) {
                        gettimeofday(&current_time, NULL);
                        int currentTime = current_time.tv_usec - startTime;
                        fprintf(fptr, "Burst added queue: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, burst->processorId, burst->pid, burst->burstLength, burst->remainingTime);
                    }
                    pthread_cond_signal(&cvs[0]);
                    pthread_mutex_unlock(&locks[0]);
                }
                else {
                    for (int i = 0; i < N; ++i)
                        pthread_mutex_lock(&locks[i]);
                    if (strcmp(QS, "RM")){
                        burst->processorId = (countRR % N) + 1;
                        
                        insert(&heads[countRR % N], burst);

                        countRR++;
                        if (OUTMODE == 3) {
                            gettimeofday(&current_time, NULL);
                            int currentTime = current_time.tv_usec - startTime;
                            printf("Burst added queue: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, burst->processorId, burst->pid, burst->burstLength, burst->remainingTime);
                        }
                        else if (OUTMODE == 6) {
                            gettimeofday(&current_time, NULL);
                            int currentTime = current_time.tv_usec - startTime;
                            fprintf(fptr, "Burst added queue: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, burst->processorId, burst->pid, burst->burstLength, burst->remainingTime);
                        }
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

                        if (OUTMODE == 3) {
                            gettimeofday(&current_time, NULL);
                            int currentTime = current_time.tv_usec - startTime;
                            printf("Burst added queue: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, burst->processorId, burst->pid, burst->burstLength, burst->remainingTime);
                        }
                        else if (OUTMODE == 6) {
                            gettimeofday(&current_time, NULL);
                            int currentTime = current_time.tv_usec - startTime;
                            fprintf(fptr, "Burst added queue: time= %d, cpu= %d, pid= %d, burstlen= %d, remainingtime = %d\n", currentTime, burst->processorId, burst->pid, burst->burstLength, burst->remainingTime);
                        }
                    }
                    for (int i = 0; i < N; ++i){
                        pthread_cond_signal(&cvs[i]);
                        pthread_mutex_unlock(&locks[i]);
                    }
                }
            }
            else if (ch == 'I'){
                k = 0;
                memset(textIAT,0,strlen(textIAT));
                ch = fgetc(ptr);
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
        fclose(ptr);
        fclose(fptr);
    }
    else{
        int count = 0;

        while(count < PC){
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
                    pthread_mutex_lock(&locks[0]);

                    burst->processorId = 1;

                    insert(&heads[0], burst);

                    headsLengths[0] = headsLengths[0] + 1;

                    pthread_cond_signal(&cvs[0]);
                    pthread_mutex_unlock(&locks[0]);
                }
                else {
                    for (int i = 0; i < N; ++i)
                        pthread_mutex_lock(&locks[i]);
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
                    for (int i = 0; i < N; ++i){
                        pthread_mutex_unlock(&locks[i]);
                        pthread_cond_signal(&cvs[i]);
                    }
                }
                usleep(xT);
            }
        }
    }

    if (SAP == 'S'){
        pthread_mutex_lock(&locks[0]);

        struct node *dummyItem = (struct node*) malloc(sizeof(struct node));
        (&dummyItem->data)->pid = -1;
        struct node *currentNode;
        struct node *prevNode;

        currentNode = heads[0];
        prevNode = heads[0];
        if(currentNode == NULL){
            dummyItem->next = NULL;
            dummyItem->prev = NULL;
            heads[0] = dummyItem;
        }
        else{
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
        pthread_cond_signal(&cvs[0]);
        pthread_mutex_unlock(&locks[0]);
    }
    else{
        for (int j = 0; j < N; ++j)
            pthread_mutex_lock(&locks[j]);
        for (int i = 0; i < N; i++)
        {
            struct node *dummyItem = (struct node*) malloc(sizeof(struct node));
            (&dummyItem->data)->pid = -1;
            struct node *currentNode;
            struct node *prevNode;

            currentNode = heads[i];
            prevNode = heads[i];
            if(currentNode == NULL){
                dummyItem->next = NULL;
                dummyItem->prev = NULL;
                heads[i] = dummyItem;
            }
            else{
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
        for (int j = 0; j < N; ++j){
            pthread_cond_signal(&cvs[j]);
            pthread_mutex_unlock(&locks[j]);
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
            count++;
        }
        curNode = 0;
        count = 0;
        curNode = list;

        while(count < minIndex){
            curNode = curNode->next;
            count++;
        }
        burst = curNode->data;

        totalTA += burst.turnaroundTime;
        countTA++;

        printf("%d\t%d\t%d\t\t%d\t%d\t%d\t\t%d\n", burst.pid, burst.processorId, burst.burstLength, burst.arrivalTime, burst.finishTime, burst.waitingTime, burst.turnaroundTime);
        deleteNode(&list,curNode);
    }
    if (countTA != 0)
        printf("average turnaround time: %d ms\n", (int)(totalTA / countTA));
}

void insert(struct node** head, struct burst_item* newItem) {
    struct node *newNode = (struct node*) malloc(sizeof(struct node));
    newNode->data = *newItem;

    struct node *current = *head;
    struct node *prev = current;

    if (*head != NULL){
        if ((*head)->data.pid < 0){
            newNode->next = *head;
            newNode->prev = NULL;
            *head = newNode;
            return;
        }
        while( *head != NULL && (*head)->data.pid > 0){
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