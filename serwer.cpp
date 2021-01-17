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
#include <list>
#include <string.h>
#include <dirent.h>
#include <iostream>

using namespace std;

list <string> songs;
const string path = "/home/karmelek/Music/";
const int one = 1;
set <int> remotes;
bool TERMI = false;


void end() {
    for (int remote : remotes) {
        close(remote);
    }
    TERMI = true;
}

void readDirectory() {
    DIR* dirpath = opendir(path.c_str());
    struct dirent* dp;
    while ((dp = readdir(dirpath)) != NULL) {
        songs.push_back(dp->d_name);
        if (songs.back() == "." || songs.back() == "..") songs.pop_back();
    }
    
    closedir(dirpath);
}

void dontStopTheMusic() {
    printf("jestem tu\n");
    
    char buffer[1024];
    int bytes_read;
    stop:
    if (songs.empty()) readDirectory();
    while (remotes.empty());
    while (!songs.empty()) {
        if (remotes.empty()) goto stop;
        string s = path + songs.front();
        songs.pop_front();
        char song[s.length()];
        strcpy(song, s.c_str());
        FILE* sound_file = fopen(song, "rb");
        while (!feof(sound_file) && !remotes.empty()) {
            if ((bytes_read = fread(&buffer, 1, 1024, sound_file)) > 0) {
            //send(clientSock, buffer, bytes_read, 0);
                for (int remote : remotes) {
                    ssize_t s = send(remote, buffer, bytes_read, 0); //sendto(remote, buffer, bytes_read, 0, (const sockaddr*) &remote, sizeof(remote));
                    if (s == -1) {
                        perror("send failed\n");
                        end();
                    }
                }
            } else {
                printf("could not get music for client - errno: %d.\n", errno);
            }
        }
        fclose(sound_file);
    }
    if (!remotes.empty()) goto stop;  
    end();
}

void addSong(char *newSong, int clientSock) {
    newSong[strcspn(newSong, "\n")] = '\0';
    char songPath[path.length()];
    strcpy(songPath, path.c_str());
    strncat(songPath, newSong, strlen(newSong));
    printf("%s\n", songPath);
    FILE* file = fopen(songPath, "wb");
    fflush(stdout);

    while (true) {
        char buf[4096];
        int datasize = recv(clientSock, buf, sizeof(buf), 0);
        if (datasize == 4096) { printf("udalo sie\n");}
        else { printf("bleble\n"); }
        fwrite(&buf, 1, datasize, file);
    } 

    fclose(file);
    string s(newSong);
    songs.insert(songs.end(), s);
    //close(clientSock);
}


void clientService(int clientSock) {
    pollfd clientDesc = { .fd = clientSock, .events = POLLIN };

    while (true) {
        int ready = poll(&clientDesc, 2, -1);
        if (ready == -1) {
            shutdown(clientSock, SHUT_RDWR);
            close(clientSock);
            error(1, errno, "poll failed!\n");
        }

        if (clientDesc.revents & POLLIN) {
            char data[10];
			int len = recv(clientSock, &data, sizeof(data), MSG_DONTWAIT);
            if (len == -1) { printf("lipa\n"); }
            else { printf("%d", len); }
            fflush(stdout);
            data[strcspn(data, "\n")] = '\0';

            if (strcmp(data, "send") == 0) {
                char answer[] = "Please give a name of the song you want to add (ONLY .WAV FILES).";
                int respond = write(clientSock, answer, strlen(answer));
                printf("wysylanie %d\n", respond);
		        if (respond == 0) {
			        printf("write failed!\n");
		        } else {
                    char songName[100];
                    fflush(stdout);
                    len = recv(clientSock, &songName, sizeof(songName), 0);
                    //mutex
                    printf("%s\n", songName);
                    addSong(songName, clientSock);
                    //mutex
                }
            }

            if (strcmp(data, "close") == 0) {
                close(clientSock);
                TERMI = true;
                break;
            }
        }  
    }
}

void handleStreaming(int clientSock) {
    printf("connected to a new music listener.\n");
    remotes.insert(clientSock);
}

void startStreaming(int serverSock) {
    sockaddr_in clientAddress;
    int clientSock;
	socklen_t clientSize = sizeof(clientAddress);
	while (true) {
		clientSock  = accept(serverSock, (sockaddr*) &clientAddress, &clientSize); 
		if (clientSock == -1){
			printf("could not connect to client, who wants to listen to music - errno: %d, but still trying.\n", errno);
        }
        handleStreaming(clientSock);
        //close(clientSock);
	}
}

void handleConnection(int clientSock) {
    printf("connected to a new client.\n");

    thread communicateWithClient(clientService, clientSock);
    communicateWithClient.detach();
}

void startConnection(int serverSock) {
    sockaddr_in clientAddress;
    int clientSock;
	socklen_t clientSize = sizeof(clientAddress);
	while (true) {
		clientSock = accept(serverSock, (sockaddr*) &clientAddress, &clientSize); 
		if (clientSock == -1){
			printf("could not connect to client - errno: %d, but still trying.\n", errno);
        }
        handleConnection(clientSock);
	}
}


int createSock(long port) {

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
    
    return serverSock;
}


int main(int argc, char ** argv){
    readDirectory();

    for (string l : songs) {
        cout << l << endl;
    }
    cout << songs.size() << endl;

    if (argc != 2)
        error(1, 0, "Usage: %s <port>", argv[0]);

    char *end;
    long port = strtol(argv[1], &end, 10);
    if (*end || port > 65535 || port < 1)
        error(1, 0, "Usage: %s <port>", argv[0]);
    
    int serverSock = createSock(port);
    thread basicConnection(startConnection, serverSock);
    
    int musicSock = createSock(14);
    thread forMusicListener(startStreaming, musicSock);
    forMusicListener.detach();
    
    thread playMusic(dontStopTheMusic);
    playMusic.detach();
    

    while (true) {
        if (TERMI == true) break;
    }

    shutdown(serverSock, SHUT_RDWR);
    close(serverSock);
    shutdown(musicSock, SHUT_RDWR);
    close(musicSock);

    return 0;

}