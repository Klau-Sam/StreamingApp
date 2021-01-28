server: server.cpp
        c++ server.cpp -Wall -pthread -lconfig++ -o server
        ./server 27 14

client: client.py
        python client.py 192.168.1.23 27 14
