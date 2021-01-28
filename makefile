server: server.cpp
	sudo apt-get install -y libconfig++-dev
	c++ server.cpp -Wall -pthread -lconfig++ -o server
	./server 27 14

client: client.py
	sudo pip install pyaudio
	python client.py 192.168.1.23 27 14
