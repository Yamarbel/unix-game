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

#define PORT 0x0da2
#define IP_ADDR 0x7f000001
#define QUEUE_LEN 20

#define THREADS_NUM 20

int main(int argc, char* argv[]) {
    struct ThreadPoolManager tpManager;
    struct Task* task;
    int listenS = socket(AF_INET, SOCK_STREAM, 0), clientInSize, newfd;
    struct sockaddr_in s = {0}, clientIn;
	if(listenS < 0) {
		perror("socket");
		return 1;
	}
    fdServer = listenS;
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = htonl(IP_ADDR);
	if(bind(listenS, (struct sockaddr*)&s, sizeof(s)) < 0) {
		perror("bind");
		return 1;
	}
	if(listen(listenS, QUEUE_LEN) < 0) {
		perror("listen");
		return 1;
	}
    clientInSize = sizeof clientIn;
    if(ThreadPoolInit(&tpManager, THREADS_NUM) < 0) {
        perror("ThreadPoolInit");
        return 1;
    }
	while(1) {
		newfd = accept(listenS, (struct sockaddr*)&clientIn, (socklen_t*)&clientInSize);
		if(newfd < 0) {
			perror("accept");
            close(listenS);
			return 1;
		}
        task = (struct Task*)malloc(sizeof(struct Task));
        if(task == NULL){
            perror("malloc");
            return 1;
        }
        task->f = threadPlay;
        task->arg = (void*)newfd;
        if(ThreadPoolInsertTask(&tpManager, task) < 0) {
            perror("ThreadPoolInsertTask");
            return 1;
        }
    }
    return 0;
}