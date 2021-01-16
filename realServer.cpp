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
#include <thread>
#include <set>

const int one = 1;
std::set<int> remotes;

void dontStopTheMusic() {
    printf("jestem tu\n");
    FILE* sound_file = fopen("/home/karmelek/Music/10. Eminem - Space Bound.wav", "rb");
    char buffer[1024];
    int bytes_read;
    while (remotes.empty());
    while (!feof(sound_file)) {
        if ((bytes_read = fread(&buffer, 1, 1024, sound_file)) > 0) {
            //send(clientSock, buffer, bytes_read, 0);
            for (int remote : remotes) {
                ssize_t s = send(remote, buffer, bytes_read, 0); //sendto(remote, buffer, bytes_read, 0, (const sockaddr*) &remote, sizeof(remote));
                if (s == -1) {
                    close(remote);
                    break;
                    perror("sendto failed");
                    
                }
            }
        } else {
            printf("could not send music to client - errno: %d.\n", errno);
        }
    }

    fclose(sound_file);
    
}

void clientService(int clientSock) {
    //thread_local int mySock = clientSock;
    /* FILE* sound_file = fopen("/home/karmelek/Music/10. Eminem - Space Bound.wav", "rb");
    size_t rret, wret;
    int bytes_read;
    char buffer[1024];
    while (!feof(sound_file)) {
        if ((bytes_read = fread(&buffer, 1, 1024, sound_file)) > 0) {
            send(clientSock, buffer, bytes_read, 0);
        } else {
            printf("could not send music to client - errno: %d.\n", errno);
        }
    }

    close(clientSock);
    fclose(sound_file); */
    FILE* gg = fopen("test5.wav", "wb");
    char buf[2048];
    while (true) {
        int datasize1 = recv(clientSock, buf, sizeof(buf), 0);
        //printf("dupa %s %u\n", datasize2, datasize1);
        if (datasize1 != 2048) {}
        else { printf("udalo sie\n"); }
        fwrite(&buf, 1, datasize1, gg);
    } 

    fclose(gg);

}


void handleConnection(int clientSock) {
    printf("connected to a new client.\n");
    remotes.insert(clientSock);

    std::thread t2(clientService, clientSock);
    t2.detach();
}

void startConnection(int serverSock) {
    sockaddr_in clientAddress;
    int clientSock;
	socklen_t clientSize = sizeof(clientAddress);
	while (true) {
		clientSock  = accept(serverSock, (sockaddr*) &clientAddress, &clientSize); 
		if (clientSock == -1){
			printf("could not connect to client - errno: %d, but still trying.\n", errno);
        }
        handleConnection(clientSock);
        //close(clientSock);
	}
}

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

    int serverSock = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSock == -1) 
        error(1, errno, "socket failed!");
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (bind(serverSock, (sockaddr*) &localAddress, sizeof(localAddress))) 
        error(1, errno, "bind failed!");

    int lis = listen(serverSock, 1);	
    if (lis) {
        error(1, errno, "listen failed!");
    } else {
        printf("waiting for connections on port %ld.\n", port);
    }

    std::thread t1(startConnection, serverSock);
    std::thread t3(dontStopTheMusic);
    t3.detach();

    while (true) {

    }

    shutdown(serverSock, SHUT_RDWR);
    close(serverSock);

    

    /*char buf[1048576];
    char datasize2[100];
    size_t datasize1;
    FILE* gg = fopen("test3.wav", "wb");
    //recv(clientSock, datasize2, sizeof(datasize2), 0);
    //size_t datasize3 = (size_t) datasize2;
    while (true) {
        datasize1 = recv(clientSock, buf, sizeof(buf), 0);
        //printf("dupa %s %u\n", datasize2, datasize1);
        if (datasize1 != 1048576) {}
        else { printf("udalo sie\n"); }
        fwrite(&buf, 1, datasize1, gg);
    } */

    

    return 0;

}