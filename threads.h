#ifndef THREADS_H
#define THREADS_H

#define TP_SIZE 64
#define STR_SIZE 256

/*needs to be initialized with the serever fd*/
static int fdServer;

struct Task {
    void* (*f)(void*);
    void* arg;
};

struct node {
    struct Task* data;
    struct node* next;
};

struct queue {
    struct node* head;
    struct node* tail;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

struct game {
    int id;
    intptr_t fdClient;
    char active;
    char goal[5];
    char guess[5];
};

struct ThreadPoolManager {
    pthread_t tPool[TP_SIZE + 1];
    struct queue tasksQ;
    struct game* gamesArr[TP_SIZE];
    int activeT;
};

/*inserts a task to the queue*/
int insertQ(struct queue* q, struct Task* t);
/*removes a task from the queue and returns it*/
struct Task* removeQ(struct queue* q);
/*initialize the thread pool*/
int ThreadPoolInit(struct ThreadPoolManager* t, int n);
/*insert a task to the queue and notify the threads a new task is avilable*/
int ThreadPoolInsertTask(struct ThreadPoolManager* t, struct Task* task);
/*destroy the thread pool*/
void ThreadPoolDestroy(struct ThreadPoolManager* t);
/*a wait function for the threads. they will wait here for the new tasks*/
void* threadWait(void* arg);
/*a function to play the game*/
void* threadPlay(void* arg);
/*a function that listens to stdin from the server*/
void* threadServer(void* arg);

#endif