#define _XOPEN_SOURCE 700

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
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
#include <dirent.h>

#include "playlists.h"


typedef struct SoundPlayer {
	char playlistID;
	FILE* sound_file;
	struct dirent** dir_entrylist;
	int dir_N;
	int dir_index;
} SoundPlayer_t;

const int one = 1;


int seekSongFile(SoundPlayer_t* player_ptr, int offset){
	//opens next sound file in the directory and closes the current one
	if (player_ptr->dir_entrylist == NULL) return 1;
	if (player_ptr->dir_N < 1) return 1;
	if (player_ptr->sound_file != NULL) fclose(player_ptr->sound_file);
	//increment & wrap index
	player_ptr->dir_index = (player_ptr->dir_index + player_ptr->dir_N + offset) % player_ptr->dir_N;
	//open the file at this index
	struct dirent* entry = player_ptr->dir_entrylist[player_ptr->dir_index];

	char path [256];
	strcpy(path, getPlaylistWithId(player_ptr->playlistID)->path);
	strcat(path, entry->d_name);


	printf("Attempting to open sound file '%s'...\n", path);

	player_ptr->sound_file = fopen(path, "rb");
//	player_ptr->sound_file = fopen("playlists/test/aa_upWord.raw", "rb");
	if (player_ptr->sound_file == NULL) return 1;
	return 0;	
}

int nextSongFile(SoundPlayer_t* player_ptr) {
	return seekSongFile(player_ptr, 1);
}

int setPlaylist(SoundPlayer_t* player_ptr) {
	//sets the player's playlist directory based on a predetermined playlist ID
	playlist_t* plist_ptr = &PLAYLISTS;
	if (plist_ptr == NULL) return 2;
	//free the old directory entry list
	for (int i = 0; i < player_ptr->dir_N; i++){
		free(player_ptr->dir_entrylist[i]);
	}
	free(player_ptr->dir_entrylist);
	//scan directory for acceptable files
	int N = scandir(plist_ptr->path, &player_ptr->dir_entrylist, NULL, alphasort);
	if (N < 1){
		player_ptr->dir_N = 0;
		player_ptr->dir_entrylist = NULL;
		return 3;
	}
	player_ptr->dir_N = N;
	//set index to negative one, setNextSongFile increments index once we enter it
	player_ptr->dir_index = -1;
	//officially set to new directory
	player_ptr->playlistID = PLAYLISTS.ID;
	int check = nextSongFile(player_ptr);
	return check;
}

int getSound(SoundPlayer_t* player_ptr, char* outBuffer){
	if (player_ptr == NULL) return 1;
	//make sure a song and playlist are being read from
	int result;
	
	if (player_ptr->dir_entrylist == NULL) { //no playlist has been chosen
		result = setPlaylist(player_ptr); //set to default playlist. sets sound file as well.
		if (result == 1) return 1;
	}
	if (player_ptr->sound_file == NULL) { //no song has been chosen (somehow)
		result = nextSongFile(player_ptr);
		if (result == 1) return 1;
	}
	
	//now a song and playlist have definitely been chosen
	int read_result = fread(outBuffer, 4, 1, player_ptr->sound_file);
	//if we couldn't read enough sound data to satisfy the request, read some from the next file
	/*while (read_result < numSamples){
		if (nextSongFile(player_ptr) != 0) return 1;
		read_result += fread(outBuffer+4*read_result, 4, numSamples-read_result, player_ptr->sound_file);
	}*/
	return 0;
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

    SoundPlayer_t player;
	player.playlistID = 0;
	player.sound_file = NULL;
	player.dir_entrylist = NULL;
	player.dir_N = 0;
	player.dir_index = 0;

    char outBuffer[259];
    int outN;
    //char ackString [5] = {0xBB, 0x02, 0xAA, 0x00, 0x01}; 

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
            if (strcmp(data, "play")) {
                printf("dupa\n");
                int result = setPlaylist(&player);
                printf("setplaylist result%d\n", result);
                outBuffer[0] = 0xBB;
                result = getSound(&player, outBuffer+1);
                printf("getsound result %d\n", result);
            }
            
            int ret = write(clientSock, outBuffer, sizeof(outBuffer));
            printf("czy wyslano %d\n", ret);
			
            //break;
		}
    }
    //close(sock);
    //shutdown(clientSock, SHUT_RDWR);
    close(clientSock);

    return 0;
}

