#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 0x0da2
#define IP_ADDR 0x7f000001

#define BUF_SIZE 256

int main(int argc, const char* argv[]) {
	int sock = socket(AF_INET, SOCK_STREAM, 0), len, sendrecv, msglen;
	struct sockaddr_in s = {0};
    char buff[BUF_SIZE] = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = htonl(IP_ADDR);
	if(connect(sock, (struct sockaddr*)&s, sizeof(s)) < 0) {
		perror("connect");
		return 1;
	}
    printf("Successfully connected.\n");
    printf("guess the number.\n");
    while (strcmp(buff,"correct!\n")) {
        fgets(buff, BUF_SIZE, stdin);
        len = strlen(buff);
        buff[len - 1] = 0;
        if (strlen(buff) != 4) {
            printf("Error: The number should contain 4 digits!\n");
        }
        else if (strchr(buff + 1 ,buff[0]) || strchr(buff + 2 ,buff[1]) || strchr(buff + 3 ,buff[2])) {
            printf("Error: All digits must be different!\n");
        }
        else {
            sendrecv = -1;
            msglen = -1;
            while((sendrecv = send(sock, buff, 5, 0)) < 5) {
                if(sendrecv < 0) {
                    perror("send");
                    return 1;
                }
            }
            if((sendrecv = recv(sock, &msglen, sizeof(int), 0)) < 0 ) {
                perror("recv");
                return 1;
            }
            if(msglen < 0) {
                printf("connection lost\n");
                close(sock);
                return 1;
            }
            while((sendrecv = recv(sock, buff, msglen, 0)) < msglen) {
                if(sendrecv < 0) {
                    perror("recv");
                    return 1;
                }
            }
            printf("%s\n", buff);
        }
    }
    close(sock);
    return 0;
}
