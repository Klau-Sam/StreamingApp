#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <poll.h>
#include <cstdlib>
#include <cstdio>

const int one = 1;

int main(int argc, char ** argv){
    if (argc != 2)
        error(1, 0, "Usage: %s <port>", argv[0]);

    char *end;
    long port = strtol(argv[1], &end, 10);
    if (*end || port > 65535 || port < 1)
        error(1, 0, "Usage: %s <port>", argv[0]);
    
    sockaddr_in localAddress {
        .sin_family = AF_INET,
        .sin_port   = htons((uint16_t) port),
        .sin_addr   = {htonl(INADDR_ANY)}
    };

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) 
        error(1, errno, "socket failed!");
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (bind(sock, (sockaddr*) &localAddress, sizeof(localAddress))) 
        error(1, errno, "bind failed!");

    int lis = listen(sock, 1);	
    if (lis) {
        error(1, errno, "listen failed!");
    } else {
        printf("waiting for connections on port %ld.\n", port);
    }

    pollfd desc[2];

    desc[0].fd = STDIN_FILENO;
    desc[0].events = POLLIN;

    desc[1].fd = sock;
    desc[1].events = POLLIN;

    sockaddr_in clientAddress;
    int clientSock;
	socklen_t clientSize = sizeof(clientAddress);

    while (true) {
        int ready = poll(desc, 2, -1);
        if (ready == -1) {
            shutdown(sock, SHUT_RDWR);
            close(sock);
            error(1, errno, "poll failed!");
        }

        if (desc[1].revents & POLLIN){
			printf("there is a client...\n");
			do {
				clientSock  = accept(sock, (sockaddr*) &clientAddress, &clientSize); 
				if (clientSock == -1){
					printf("could not connect to client - errno: %d, but still trying.\n", errno);
				}
			} while (clientSock == -1);

			printf("connected.\n");
            break;

            /*while (true) {
                char* data;
                printf("jestem tu");
                int len = read(clientSock, data, sizeof(data));
                if (len < 1) break;
                printf(" Received %2d bytes: |%s|\n", len, data);
            }*/
		}
    }

    pollfd clientdesc = { .fd = clientSock, .events = POLLIN };
    shutdown(sock, SHUT_RDWR);
    close(sock);

    while (true) {
        int ready = poll(&clientdesc, 1, -1);
        if (ready == -1) {
            printf("tralalala");
            close(clientSock);
            error(1, errno, "poll failed!");
        }

        if (clientdesc.revents & POLLIN) {
            char data[10];
			int len = recv(clientSock, &data, sizeof(data), MSG_DONTWAIT);
            int ret = write(clientSock, data, sizeof(data));
			printf("Halo");
            //break;
		}
    }
    //close(sock);
    //shutdown(clientSock, SHUT_RDWR);
    close(clientSock);

    return 0;
}