#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include "threads.h"

/*incrimented id for the games*/
static int currentGame = 1;

int insertQ(struct queue* q, struct Task* t) {
    if(q->head == NULL) {
        q->head = (struct node*)malloc(sizeof(struct node));
        if(q->head == NULL) {
            perror("insert head malloc");
            return -1;
        }
        q->head->data = t;
        q->head->next = NULL;
        q->tail = q->head;
        q->size = 1;
        return 0;
    }
    else {
        q->tail->next = (struct node*)malloc(sizeof(struct node));
        if(q->tail->next == NULL) {
            perror("insert malloc");
            return -2;
        }
        q->tail = q->tail->next;
        q->tail->data = t;
        q->tail->next = NULL;
        q->size++;
        return 0;
    }
}

struct Task* removeQ(struct queue* q) {
    struct Task* toReturn;
    struct node* toDelete;
    if(q->size < 1 || q->head == NULL) {
        return NULL;
    }
    toReturn = q->head->data;
    toDelete = q->head;
    q->head = q->head->next;
    free(toDelete);
    q->size--;
    return toReturn;
}

int ThreadPoolInit(struct ThreadPoolManager* t, int n) {
    int i;
    if(n > TP_SIZE || n < 1) {
        perror("number of threads");
        return -1;
    }
    if(pthread_mutex_init(&t->tasksQ.lock, NULL)) {
        perror("mutex init");
        return -2;
    }
    if(pthread_cond_init(&t->tasksQ.cond, NULL)) {
        perror("cond init");
        return -3;
    }
    for(i = 0; i < n; i++) {
        t->gamesArr[i] = (struct game*)malloc(sizeof(struct game));
        if(t->gamesArr[i] == NULL) {
            perror("game malloc");
            return -4;
        }
        t->gamesArr[i]->active = 0;
        if(pthread_create(&t->tPool[i], NULL, threadWait, (void*)t)) {
            perror("thread create");
            return -6;
        }
    }
    t->tasksQ.size = 0;
    t->tasksQ.head = NULL;
    t->tasksQ.tail = NULL;
    t->activeT = n;
    if(pthread_create(&t->tPool[TP_SIZE], NULL, threadServer, (void*)t)) {
        perror("server thread create");
        return -7;
    }
    return 0;
}

int ThreadPoolInsertTask(struct ThreadPoolManager* t, struct Task* task) {
    if(pthread_mutex_lock(&t->tasksQ.lock)) {
        perror("lock");
        return -1;
    }
    if(insertQ(&t->tasksQ, task)) {
        return -2;
    }
    if(pthread_cond_broadcast(&t->tasksQ.cond)) {
        perror("broadcast");
        return -3;
    }
    if(pthread_mutex_unlock(&t->tasksQ.lock)) {
        perror("unlock");
        return -4;
    }
    return 0;
}

void ThreadPoolDestroy(struct ThreadPoolManager* t) {
    int i;
    if(pthread_mutex_lock(&t->tasksQ.lock)) {
        perror("lock");
        return;
    }
    while(t->tasksQ.size > 0) {
        removeQ(&t->tasksQ);
    }
    if(pthread_mutex_unlock(&t->tasksQ.lock)) {
        perror("unlock");
        return;
    }
    for(i = 0; i < t->activeT; i++) {
        if(t->gamesArr[i]->active == '1') {
            shutdown(t->gamesArr[i]->fdClient, SHUT_RDWR);
            close(t->gamesArr[i]->fdClient);
        }
        free(t->gamesArr[i]);
        pthread_cancel(t->tPool[i]);
    }
    pthread_cond_destroy(&t->tasksQ.cond);
    pthread_mutex_destroy(&t->tasksQ.lock);
}

void* threadWait(void* arg) {
    struct ThreadPoolManager* tpm = (struct ThreadPoolManager*)arg;
    struct Task* temp;
    struct Task t;
    int i;
    pthread_t myid;
    while(1) {
        pthread_mutex_lock(&tpm->tasksQ.lock);
        while(tpm->tasksQ.size < 1) {
            pthread_cond_wait(&tpm->tasksQ.cond, &tpm->tasksQ.lock);
        }
        temp = removeQ(&tpm->tasksQ);
        if(!temp) {
            return NULL;
        }
        t.f = temp->f;
        t.arg = temp->arg;
        free(temp);
        myid = pthread_self();
        for(i = 0; pthread_equal(myid, tpm->tPool[i]) == 0; i++);
        tpm->gamesArr[i]->fdClient = (intptr_t)t.arg;
        t.arg = (void*)tpm->gamesArr[i];
        pthread_mutex_unlock(&tpm->tasksQ.lock);
        t.f(t.arg);
    }
}

void* threadPlay(void* arg) {
    struct game* g = (struct game*)arg;
    char done = 0, result[100];
    char* check;
    int i, bulls, cows, sendrecv = 0, len;
    srand(time(NULL));
    fflush(stdout);
    g->goal[0] = rand()%10 + '0';
    do {
        g->goal[1] = rand()%10 + '0';
    } while (g->goal[1] == g->goal[0]);
    do {
        g->goal[2] = rand()%10 + '0';
    } while (g->goal[2] == g->goal[1] || g->goal[2] == g->goal[0]);
    do {
        g->goal[3] = rand()%10 + '0';
    } while (g->goal[3] == g->goal[2] || g->goal[3] == g->goal[1] || g->goal[3] == g->goal[0]);
    g->goal[4] = '\0';
    g->id = currentGame++;
    g->active = '1';
    while(!done) {
        bulls = 0;
        cows = 0;
        while((sendrecv = recv(g->fdClient, g->guess, 5, 0)) < 5);
        g->guess[4] = '\0';
        for(i = 0; i < 4; i++) {
            check = strchr(g->goal, g->guess[i]);
            if (check != NULL)
            {
                if (g->guess[i] == g->goal[i])
                    bulls++;
                else
                    cows++;
            }
        }
        if(bulls == 4) {
            len = strlen("correct!\n") + 1;
            send(g->fdClient, &len, sizeof(int), 0);
            while((sendrecv = send(g->fdClient, "correct!\n", len, 0)) < len);
            done = '1';
        }
        else {
            sprintf(result,"bulls = %d, cows = %d\n", bulls, cows);
            len = strlen(result) + 1;
            send(g->fdClient, &len, sizeof(int), 0);
            while((sendrecv = send(g->fdClient, result, len, 0)) < len);
        }
    }
    close(g->fdClient);
    g->active = 0;
}

void* threadServer(void* arg) {
    struct ThreadPoolManager* tpm = (struct ThreadPoolManager*)arg;
    char command[STR_SIZE] = {0};
    int len, i;
    while(1) {
        fgets(command, STR_SIZE, stdin);
        len = strlen(command);
        command[len - 1] = 0;
        if(!strcmp(command, "list")) {
            for(i = 0; i < tpm->activeT; i++) {
                if(tpm->gamesArr[i]->active == '1') {
                    printf("Game %d - Number: %s, Last Guess: %s\n", tpm->gamesArr[i]->id, tpm->gamesArr[i]->goal, tpm->gamesArr[i]->guess);
                }
            }
        }
        else if(!strcmp(command, "quit")) {
            ThreadPoolDestroy(tpm);
            shutdown(fdServer, SHUT_RDWR);
            close(fdServer);
            exit(0);
        }
    }
}