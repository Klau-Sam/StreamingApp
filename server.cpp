#include <sys/types.h>
#include <sys/socket.h> //for networking
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h> //unix standard, for read system call
#include <stdlib.h> //c standard: for string parsing
#include <error.h>
#include <errno.h>
#include <thread>
#include <set>
#include <string.h>
#include <dirent.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <poll.h>
#include <libconfig.h++>

using namespace std;
using namespace libconfig;

vector <string> songs;  
const int one = 1;
set <int> remotes;
set <int> sockets;
string playlist = "";
atomic<bool> permission(false);
atomic<bool> questionnaire(false);
atomic<bool> changes(false);
mutex mtx1;
mutex mtx2;
atomic<bool> mtx2isLocked(false);
mutex mtx3;
atomic<bool> mtx3isLocked(false);

string returnPath() {
    Config cfg;
    try {
        cfg.readFile("stream.cfg");
    } catch (const FileIOException &fioex) {
        cerr << "I/O error while reading configuration file." << endl;
        exit(1);
    } catch (const ParseException &pex) {
        cerr << "Parse error at" << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << endl;
        exit(1);
    }

    string directoryPath;
    try {
        directoryPath = cfg.lookup("path").c_str();
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'path' in configuration file" << endl;
        exit(1);
    }

    return directoryPath;
}

void readDirectory(string directoryPath) {
    DIR* dirpath = opendir(directoryPath.c_str()); //125
    struct dirent* dp;
    while ((dp = readdir(dirpath)) != NULL) {
        if (dp->d_type == DT_DIR) {
            continue;
        }
        string tmp_string(dp->d_name);
        int position = tmp_string.find_last_of(".");
        string result = tmp_string.substr(position + 1);
        if (strcmp(result.c_str(), "wav") != 0) {
            continue;
        }
        songs.push_back(dp->d_name);
        if (songs.back() == "." || songs.back() == "..") {
            songs.pop_back();
            continue;
        }
    }
    if (songs.empty()) {
        cout << "There is an error with the directory." << endl;
    } else {
        changes = true;
    }
    closedir(dirpath);
}

//void dontStopTheMusic(string directoryPath) { }

void getPlaylist() {
    if (changes == true) {
        playlist = "";
        for (size_t i = 0; i < songs.size(); i++) {
            playlist = playlist + to_string(i+1) + " " + songs.at(i);
            if (i != songs.size()-1) playlist = playlist + "\n";
        }
        changes = false;
    }
}

int skipSong(char *skipSong, int clientSock) {
    size_t skip = atoi(skipSong);
    if (skip <= 1 || skip > songs.size()) {
        char answer[] = "Wrong input value, try again.";
        int respond = write(clientSock, answer, strlen(answer));
        if (respond <= 0) {
            if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                cout << "write failed!" << endl;
                close(clientSock);
            }
		}
        return 1;
    }
    for (size_t j = 0; j < (skip-1); j++) {
        songs.erase(songs.begin());
    }

    changes = true;
    char answer[] = "Request successfully submitted.";
    int respond = write(clientSock, answer, strlen(answer));
    if (respond <= 0) {
        if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
            cout << "write failed!" << endl;
            close(clientSock);
        }
	}
    return 0;
}

int changeOrder(char *newOrder, int clientSock) {
    char *pch = strtok(newOrder, " ");
    set <int> temp;
    vector <string> temp2;
    while (pch != NULL) {
        size_t i = atoi(pch);
        if (i < 1 || i > songs.size()) { 
            char answer[] = "Wrong input values, try again. Or write exit to exit.";
            int respond = write(clientSock, answer, strlen(answer));
            if (respond <= 0) {
                if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                    cout << "write failed!" << endl;
                    close(clientSock);
                    break;
                }
		    }
            return 1;
        }
        temp.insert(i);
        temp2.push_back(songs.at(i-1));
        pch = strtok(NULL, " ");
    }

    if (temp.size() != songs.size()) {
        char answer[] = "Wrong input values, try again. Or write exit to exit.";
        int respond = write(clientSock, answer, strlen(answer));
        if (respond <= 0) {
            if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                cout << "write failed!" << endl;
                close(clientSock);
            }
		}
        return 1;
    } else {
        songs = temp2;
        changes = true;
        char answer[] = "Request successfully submitted.";
        int respond = write(clientSock, answer, strlen(answer));
        if (respond <= 0) {
            if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                cout << "write failed!" << endl;
                close(clientSock);
            }
		}
        //sockets.erase(clientSock);
        //questionnaire = true;
    }

    return 0;
}

int checkTheName(char *newSong, int clientSock) {
    string temp(newSong);
    if (temp.find("/") != string::npos || temp.find_first_of(".") == 0 || temp.find_first_of(".") == 1 || temp.find_first_of(".") == string::npos) {
        char answer[] = "That's the terrible wrong name of the song. Write new one or exit to exit.";
        int respond = write(clientSock, answer, strlen(answer));
        if (respond <= 0) {
            if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                cout << "write failed!" << endl;
            }
		}
        return 1;
    }

    int position = temp.find_last_of(".");
    string result = temp.substr(position + 1);
    if (strcmp(result.c_str(), "wav") == 0) {
        for (string song : songs) {
            if (strcmp(song.c_str(), newSong) == 0) {
                songs.push_back(song);
                changes = true;
                char answer[] = "Song already exists in our base and is added at the end of queue.";
                int respond = write(clientSock, answer, strlen(answer));
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                    cout << "write failed!" << endl;
                    }
		        }
                return 2;
            }
        }
        char answer[] = "Everything's fine.";
        int respond = write(clientSock, answer, strlen(answer));
        if (respond <= 0) {
            if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                cout << "write failed!" << endl;
            }
		}
        return 0;
    }
    
    char answer[] = "That's the wrong name of the song. Write new one or exit to exit.";
    int respond = write(clientSock, answer, strlen(answer));
    if (respond <= 0) {
        if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
            cout << "write failed!" << endl;
        }
	}
    return 1;
}

void addSong(char *newSong, char *sizeOfSong, int clientSock) {
    string path = returnPath();
    int size = atoi(sizeOfSong);
    char songPath[path.length() + strlen(newSong)];
    strcpy(songPath, path.c_str());
    strncat(songPath, newSong, strlen(newSong));
    FILE* file = fopen(songPath, "wb");
    if (file != NULL) {
        fflush(stdout);

        while (size != 0) {
            char buf[16];
            bzero(buf, sizeof(buf));
            int dataSize = recv(clientSock, buf, sizeof(buf), 0);
            if (dataSize == 0 || dataSize == -1) {
                break;
            }
            size -= dataSize;
            cout << size << endl;
            fwrite(&buf, 1, dataSize, file);
        } 
        if (size == 0) {
            string s(newSong);
            mtx2.lock();
            songs.insert(songs.end(), s);
            changes = true;
            mtx2.unlock();
        }
        fclose(file);    
    } else {
        cout << "Problem with creating a new file, song cannot be added." << endl;
    }
}


void clientService(int clientSock) {
    getPlaylist();
    int welcome = write(clientSock, playlist.c_str(), playlist.length());
    if (welcome <= 0) {
            cout << "welcome message didn't send!" << endl;
	}

    while (true) {
        //fflush(stdout);
        char data[10];
        bzero(data, sizeof(data));
        int ready = recv(clientSock, &data, sizeof(data), 0);
        if (ready == 0 || ready == -1) {
            cout << "Closing client's communication socket." << endl;
            close(clientSock);
            break;
        } else { cout << "Number of bytes received: " << ready << endl; }
            
        fflush(stdout);
        data[strcspn(data, "\n")] = '\0';
        cout << "Arrived command: " << data << endl;

        if (strcmp(data, "send") == 0) {
            //char sync[] = "100";
            //int respond = write(clientSock, sync, strlen(sync));
            char answer[] = "Please give a name of the song you want to add (ONLY .WAV FILES).";
            int respond = write(clientSock, answer, strlen(answer));
		    if (respond <= 0) {
                if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                    cout << "write failed!" << endl;
                    close(clientSock);
                    break;
                }
		    }
                
            char songName[100];
            bzero(songName, sizeof(songName));
            int len = recv(clientSock, &songName, sizeof(songName), 0);
            if (len == 0 || len == -1) {
                cout << "Closing client's communication socket." << endl;
                close(clientSock);
                break;
            }
            songName[strcspn(songName, "\n")] = '\0';
            cout << songName << endl;
            int check = checkTheName(songName, clientSock);
            while (check == 1) {
                bzero(songName, sizeof(songName));
                len = recv(clientSock, &songName, sizeof(songName), 0);
                if (len == 0 || len == -1) {
                    cout << "Closing client's communication socket." << endl;
                    close(clientSock);
                    break;
                }
                songName[strcspn(songName, "\n")] = '\0';
                if (strcmp(songName, "exit") == 0) break;
                check = checkTheName(songName, clientSock);
            }   
            if (check == 0) {
                char sizeOfSong[10];
                len = recv(clientSock, &sizeOfSong, sizeof(sizeOfSong), 0);
                if (len == 0 || len == -1) {
                    cout << "Closing client's communication socket." << endl;
                    close(clientSock);
                    break;
                }
                mtx3.lock();
                mtx3isLocked = true;
                addSong(songName, sizeOfSong, clientSock);
                mtx3.unlock();
                mtx3isLocked = false;
            }
        }

        if (strcmp(data, "edit") == 0) {
            //char sync[] = "101";
            //int respond = write(clientSock, sync, strlen(sync));
            mtx1.lock();
            if (!permission.load()) {
                permission = true;
                mtx1.unlock();
                char answer[] = "Please write new order of songs in playlist (SEPARATED BY WHITESPACES).";
                int respond = write(clientSock, answer, strlen(answer));
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                        cout << "write failed!" << endl;
                        close(clientSock);
                        break;
                    }
		        }
                this_thread::sleep_for(500ms);
                mtx2.lock();
                mtx2isLocked = true;
                getPlaylist();
                respond = write(clientSock, playlist.c_str(), playlist.length());
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                        cout << "write failed!" << endl;
                        mtx2.unlock();
                        mtx2isLocked = false;
                        close(clientSock);
                        break;
                    }
		        }
                fflush(stdout);
                char newOrder[songs.size()*4];
                int len = recv(clientSock, &newOrder, sizeof(newOrder), 0);
                if (len == 0 || len == -1) {
                    cout << "Closing client's communication socket." << endl;
                    mtx2.unlock();
                    mtx2isLocked = false;
                    close(clientSock);
                    break;
                }

                int c = changeOrder(newOrder, clientSock);
                while (c == 1) {
                    bzero(newOrder, sizeof(newOrder));
                    len = recv(clientSock, &newOrder, sizeof(newOrder), 0);
                    if (len == 0 || len == -1) {
                        cout << "Closing client's communication socket." << endl;
                        mtx2.unlock();
                        mtx2isLocked = false;
                        close(clientSock);
                        break;
                    }
                    newOrder[strcspn(newOrder, "\n")] = '\0';
                    if (strcmp(newOrder, "exit") == 0) break;
                    cout << len << newOrder << endl;
                    c = changeOrder(newOrder, clientSock);
                }
                permission = false;
                mtx2.unlock();
                mtx2isLocked = false;
            } else {
                mtx1.unlock();
                char answer[] = "Sorry, someone else is changing the order now. Try later.";
                int respond = write(clientSock, answer, strlen(answer));
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                        cout << "write failed!" << endl;
                        close(clientSock);
                        break;
                    }
		        }
            }
        }

        if (strcmp(data, "skip") == 0) {
            //char sync[] = "105";
            //int respond = write(clientSock, sync, strlen(sync));
            mtx1.lock();
            if (!permission.load()) {
                permission = true;
                mtx1.unlock();
                char answer[] = "Please write which song you want to jump to after current song ends (ONLY NUMBER ON A LIST).";
                int respond = write(clientSock, answer, strlen(answer));
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                        cout << "write failed!" << endl;
                        close(clientSock);
                        break;
                    }
		        }
                this_thread::sleep_for(500ms);
                mtx2.lock();
                mtx2isLocked = true;
                getPlaylist();
                respond = write(clientSock, playlist.c_str(), playlist.length());
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                        cout << "write failed!" << endl;
                        mtx2.unlock();
                        mtx2isLocked = false;
                        close(clientSock);
                        break;
                    }
		        }
                fflush(stdout);
                char kickSong[5];
                int len = recv(clientSock, &kickSong, sizeof(kickSong), 0);
                if (len == 0 || len == -1) {
                    cout << "Closing client's communication socket." << endl;
                    mtx2.unlock();
                    mtx2isLocked = false;
                    close(clientSock);
                    break;
                }
                int c = skipSong(kickSong, clientSock);
                while (c == 1) {
                    bzero(kickSong, sizeof(kickSong));
                    len = recv(clientSock, &kickSong, sizeof(kickSong), 0);
                    if (len == 0 || len == -1) {
                        cout << "Closing client's communication socket." << endl;
                        mtx2.unlock();
                        mtx2isLocked = false;
                        close(clientSock);
                        break;
                    }
                    kickSong[strcspn(kickSong, "\n")] = '\0';
                    if (strcmp(kickSong, "exit") == 0) { break; }
                    cout << len << kickSong << endl;
                    c = skipSong(kickSong, clientSock);
                }
                permission = false;
                mtx2.unlock();
                mtx2isLocked = false;
            } else {
                mtx1.unlock();
                char answer[] = "Sorry, someone else is changing the order now. Try later.";
                int respond = write(clientSock, answer, strlen(answer));
                if (respond <= 0) {
                    if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                        cout << "write failed!" << endl;
                        close(clientSock);
                        break;
                    }
		        }
            }
        }

        if (strcmp(data, "list") == 0) {
            //char sync[] = "104";
            //int respond = write(clientSock, sync, strlen(sync));
            getPlaylist();
            int respond = write(clientSock, playlist.c_str(), playlist.length());
            if (respond <= 0) {
                if (errno == ECONNRESET || errno == EPIPE || errno == EACCES || errno == ENETDOWN) {
                    cout << "write failed!" << endl;
                    close(clientSock);
                    break;
                }
		    }
        }

        if (strcmp(data, "quit") == 0) {
            close(clientSock);
            break;
        } 
    }
}

/* void checkClientActivity() {
    char buf[] = "";
    int respond;
    while (true) {
        if (mtx2isLocked == true || mtx2isLocked == true) {
            this_thread::sleep_for(90000ms);
        }
        if (mtx2isLocked == true) {
            for (int s : sockets) {
                if ((respond = write(s, buf, sizeof(buf))) == -1) {
                    if (errno == ECONNRESET) {
                        mtx2.unlock();
                        mtx2isLocked = false;
                        sockets.erase(s);
                        close(s);
                    }
                }
            }
        }

        if (mtx3isLocked == true) {
            for (int s : sockets) {
                if ((respond = write(s, buf, sizeof(buf))) == -1) {
                    if (errno == ECONNRESET) {
                        mtx3.unlock();
                        mtx3isLocked = false;
                        sockets.erase(s);
                        close(s);
                    }
                }
            }
        }
    } 
} */

void handleStreaming(int clientSock) {
    remotes.insert(clientSock);
    printf("Connected to a new music listener.\n");
}

void startStreaming(int serverSock) {
    sockaddr_in clientAddress;
    int clientSock;
	socklen_t clientSize = sizeof(clientAddress);
	while (true) {
		clientSock  = accept(serverSock, (sockaddr*) &clientAddress, &clientSize); 
		if (clientSock < 0){
			//printf("could not connect to client, who wants to listen to music - errno: %d, but still trying.\n", errno);
            if ((errno == ENETDOWN || errno == EPROTO || errno == ENOPROTOOPT || errno == EHOSTDOWN || errno == ENONET || errno == EHOSTUNREACH || errno == EOPNOTSUPP || errno == ENETUNREACH)) {
                continue;
            }
            error(1, errno, "accept failed");
        }
        handleStreaming(clientSock);
	}
}

void handleConnection(int clientSock) {
    printf("Connected to a new client.\n");

    //options[pollopt].fd = clientSock;
    //options[pollopt].events = POLLIN;
    //pollopt++;
    sockets.insert(clientSock);
    thread communicateWithClient(clientService, clientSock);
    communicateWithClient.detach();
}

void startConnection(int serverSock) {
    sockaddr_in clientAddress;
    int clientSock;
	socklen_t clientSize = sizeof(clientAddress);
	while (true) {
		clientSock = accept(serverSock, (sockaddr*) &clientAddress, &clientSize); 
        //if (setsockopt(serverSock, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(one))) {};
		if (clientSock < 0){
			// printf("could not connect to client - errno: %d, but still trying.\n", errno);
            if ((errno == ENETDOWN || errno == EPROTO || errno == ENOPROTOOPT || errno == EHOSTDOWN || errno == ENONET || errno == EHOSTUNREACH || errno == EOPNOTSUPP || errno == ENETUNREACH)) {
                continue;
            }
            error(1, errno, "accept failed");

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
        printf("Waiting for connections on port %ld.\n", port);
    }
    
    return serverSock;
}


int main(int argc, char ** argv){
    if (argc != 3)
        error(1, 0, "Usage: %s <port for communication> <port for streaming>", argv[0]);

    char *end1;
    char *end2;
    long port1 = strtol(argv[1], &end1, 10);
    long port2 = strtol(argv[2], &end2, 10);
    if (*end1 || *end2 || port1 > 65535 || port1 < 1 || port2 > 65535 || port2 < 1)
        error(1, 0, "Usage: %s <port for communication> <port for streaming>", argv[0]);
    
    int serverSock = createSock(port1);
    thread basicConnection(startConnection, serverSock);
    basicConnection.detach();
    
    int musicSock = createSock(port2);
    thread forMusicListener(startStreaming, musicSock);
    forMusicListener.detach();

    //thread checkingClients(checkClientActivity);
    //checkingClients.detach();
    //thread playMusic(dontStopTheMusic, directoryPath);
    //playMusic.detach();
    
    string directoryPath = returnPath();
    
    while (true) {
        cout << "Please don't stop the music :)" << endl;
    
        char soundBuff[32];
        int bytesRead;

        stop:
        if (songs.empty()) readDirectory(directoryPath);
        //while (remotes.empty());
        while (!songs.empty()) {
            if (remotes.empty()) goto stop;
            mtx2.lock();
            string s = directoryPath + songs.at(0);
            songs.erase(songs.begin());
            changes = true;
            mtx2.unlock();
            FILE* soundFile = fopen(s.c_str(), "rb");
            if (soundFile == NULL) {
                cerr << "Problem with the song " << s << ", trying next one." << endl;
                fclose(soundFile);
                goto stop;
            }
            while (!feof(soundFile) && !remotes.empty()) {
                if ((bytesRead = fread(&soundBuff, 1, 32, soundFile)) > 0) {
                    for (int remote : remotes) {
                        ssize_t s = write(remote, soundBuff, bytesRead);//send(remote, soundBuff, bytesRead, 0); //sendto(remote, buffer, bytes_read, 0, (const sockaddr*) &remote, sizeof(remote));
                        if (s == -1) {
                            printf("Music client is gone.\n");
                            remotes.erase(remote);
                            close(remote);
                        }
                        if (remotes.empty()) {
                            break;
                        }
                    }
                } else {
                    cout << "Could not get a bit of music for client." << endl;
                }
            }
            fclose(soundFile);
        }
        if (!remotes.empty()) goto stop;  
    }

    shutdown(serverSock, SHUT_RDWR);
    close(serverSock);
    shutdown(musicSock, SHUT_RDWR);
    close(musicSock);

    return 0;
}

//mutex1
        /* if (questionnaire == true) {
            printf("here\n");
            pollfd options[sockets.size()];
            int n = 0;
            for (int soc : sockets) {
                options[n].fd = soc;
                printf("%d\n", soc);
                options[n].events = POLLIN;
                n++;
            }
            vector <string> copyOfSongs = songs;
            for (pollfd &p : options) {
                char sync[] = "103";
                int s = write(p.fd, sync, sizeof(sync));
                printf("wyslano\n");
            }
            char buff[] = "Do you agree with this change? (SEND Y OR N).\n";
            sleep(2);
            printPlaylist();
            for (pollfd &p : options) {
                int t = write(p.fd, playlist.c_str(), playlist.length());
                int s = write(p.fd, buff, sizeof(buff));
            } 
 
            while (true) {
                int ready = poll(options, sockets.size(), -1);
                for (pollfd &p : options) {
                    if (p.revents & POLLIN) {
                        char answer[5];
                        read(p.fd, answer, 5);
                        answer[strcspn(answer, "\n")] = '\0';
                        printf("%s\n", answer);
                    }
                    if (ready == 0) {
                        break;
                    }
                }
                if (ready == 0) {
                    break;
                }
            }
            printf("wyszeedlem\n");
            questionnaire = false;
        } */
        
        //end();