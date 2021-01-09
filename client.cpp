#include <sys/types.h>
#include <sys/socket.h> //for networking
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h> //unix standard, for read system call
#include <stdlib.h> //c standard: for string parsing
#include <poll.h>
#include <error.h>
#include <errno.h>

int menu(char* message, int n, int sock) {
    message[strcspn(message, "\n")] = '\0';
    if (strcmp(message, "play") == 0) {
        int respond = write(sock, message, strlen(message));
        printf("%d\n", respond);
		if (respond == 0) {
			printf("write failed!");
			return 1;
		}
    }
    return 0;
}

int main(int argc, char ** argv){
    if (argc != 3)
        error(1, 0, "Usage: %s <ip> <port>", argv[0]);
    
    addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    
    addrinfo *resolved;
    if (int err = getaddrinfo(argv[1], argv[2], &hints, &resolved))
        error(1, 0, "Resolving address failed: %s", gai_strerror(err));
    
    int sock = socket(resolved->ai_family, resolved->ai_socktype, resolved->ai_protocol);
    if (sock == -1) {
        error(1, errno, "socket failed!");
        return 1;
    }

    if (connect(sock, resolved->ai_addr, resolved->ai_addrlen))
        error(1, errno, "connect failed");
    printf("connected to server.\n");
    freeaddrinfo(resolved);
    
    pollfd desc[2];

    desc[0].fd = STDIN_FILENO;
    desc[0].events = POLLIN;

    desc[1].fd = sock;
    desc[1].events = POLLIN;

    while (true) {
        poll(desc, 2, -1);

        if (desc[0].revents & POLLIN){
            char message[256];
			int len = read(STDIN_FILENO, message, 256);
            printf("Received console command...\n");
            printf("wielkossc n %d\n", len);
            printf("wielkosc strlen %d\n", strlen(message));
			int command = menu(message, len, sock);
		}

        if (desc[1].revents & POLLIN) {
            char mess[256];
			int len = read(sock, mess, 256);
            fflush(stdout);
            if (len > 0) printf("Received data from server: ");
			for (int i=0;i<len;i++) printf("%02X ", (unsigned char)mess[i]);
			printf("\n");
        }
    }

    /*for (char letter = 'a'; letter <= 'z' ; ++letter){
        write(sock, &letter, 1);
    } */
    
    shutdown(sock, SHUT_RDWR);
    close(sock);
    return 0;
}


