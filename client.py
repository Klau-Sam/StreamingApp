import socket  # Import socket module
import pyaudio
import sys
import binascii
from _thread import *
import threading


def createStream(Music):
    stream = Music.open(
        format=8,
        channels=2,
        rate=44100,
        output=True
    )
    return stream


def connect2(s,port):
    # host = input()
    # port = int(input())
    # host = "127.0.0.1"
    # host = "karmelek-Inspiron-N5010"
    host = "192.168.1.23"
    # host = "localhost"
    # host = "127.0.0.1"

    # host = "localhost"
    s.connect((host, port))
    # s.send('play'.encode())
    print("connected")

def run(s, stream):
    data = s.recv(1024)
    print(data)
    while data != '':
        stream.write(data)

        data = s.recv(1024)
    stream.close()


def send(sock):
    filename = 'test.wav'  # In the same folder or path is this file running must the file you want to tranfser to be
    f = open(filename, 'rb')
    l = f.read(2048)
    # print(l)
    # size=(sys.getsizeof(l))
    # print(size)
    # print(size.to_bytes(8, byteorder='big'))
    while (l):
        sock.send(l)
        print('Sent ', repr(l))
        l = f.read(2048)
    f.close()

    print('Done sending')


def main():
    silence = True
    communicationSocket = socket.socket()  # creates socket
    connect2(communicationSocket, 80)  # connects socket via hostname and port from input arguments
    while True:
        # test()


        a = input()
        if a == 'send':
            send(communicationSocket)
        if a == 'play':

            if silence:
                playSocket = socket.socket()  # creates socket
                connect2(playSocket, 14)
                Music = pyaudio.PyAudio()  # initiate pyaudio variable
                stream = createStream(Music)
                start_new_thread(run, (playSocket, stream))
                silence = False
            else:
                #Music.terminate()
                playSocket.close()


        if a == 'quit':
            Music.terminate()










def test():
    filename = 'test.wav'  # In the same folder or path is this file running must the file you want to tranfser to be
    f = open(filename, 'rb')
    l = f.read(1024)
    # print(l)
    size = (sys.getsizeof(l))
    print(size)
    # print(size.to_bytes(2, byteorder='big'))


# sock.send(size.to_bytes(8, byteorder='big'))

if __name__ == "__main__":
    main()
