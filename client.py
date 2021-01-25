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


def connect2(s, port):
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
    data = s.recv(16)
    # print(data)
    playing = data

    while data != '':
        stream.write(data)

        data = s.recv(16)
    stream.close()


def sendFile(sock, songname):
    filename = songname  # In the same folder or path is this file running must the file you want to tranfser to be
    f = open(filename, 'rb')
    l = f.read(512)
    # print(l)
    # size=(sys.getsizeof(l))
    # print(size)
    # print(size.to_bytes(8, byteorder='big'))
    while (l):
        sock.send(l)
        # print('Sent ', repr(l))
        l = f.read(512)
    f.close()


def sendMessage(sock, message):
    sock.send(message)


def main():
    silence = True
    communicationSocket = socket.socket()  # creates socket
    connect2(communicationSocket, 27)  # connects socket via hostname and port from input arguments
    init = True
    while True:

        if init == True:
            message = communicationSocket.recv(1024)
            message = str(message)
            message = message.replace(r'\n', '\n')
            print(message)
            init = False

        print('test')
        a = input()

        if a == 'send':
            print('you want to upload new song.')
            sendMessage(communicationSocket, 'send'.encode())
            message = communicationSocket.recv(1024)
            print(message)
            print(
                "Write a name of your song(only .wav format, make sure its in your folder and to include .wav at the end")
            songname = input()
            sendMessage(communicationSocket, songname.encode())
            sendFile(communicationSocket, songname)
            sendMessage(communicationSocket, "end".encode())
            print("song sent.")
        if a == 'play':
            print('playing')
            if silence:
                playSocket = socket.socket()  # creates socket
                connect2(playSocket, 14)
                Music = pyaudio.PyAudio()  # initiate pyaudio variable
                stream = createStream(Music)
                start_new_thread(run, (playSocket, stream))
                print("out of thread")
                silence = False
            else:
                # Music.terminate()
                playSocket.close()

        if a == 'skip':
            print('you want to skip some songs.')
            sendMessage(communicationSocket, 'skip'.encode())
            message = communicationSocket.recv(1024)
            print(message)
            if "Sorry".encode() not in message:
                print("Current song order:")
                message = communicationSocket.recv(1024)
                message = message.replace(r'\n', '\n')
                print(message)
                print('how many songs you want to skip')
                songSkip = input()
                sendMessage(communicationSocket, songSkip.encode())
                message = communicationSocket.recv(1024)
                print(message)
                while "Wrong input values".encode() in message:
                    songSkip = input()
                    sendMessage(communicationSocket, songSkip.encode())
                    message = communicationSocket.recv(1024)
                    print(message)
                print('successfully skipped ', songSkip, 'songs')
        if a == 'quit':
            Music.terminate()

        if a == 'list':
            print('receiving list of songs:')
            sendMessage(communicationSocket, 'list'.encode())
            message = communicationSocket.recv(1024)
            message = str(message)
            message = message.replace(r'\n', '\n')
            print(message)

        if a == 'edit':
            print('you want to edit playlist.')
            sendMessage(communicationSocket, 'edit'.encode())
            message = communicationSocket.recv(1024)
            print(message)
            if "Sorry".encode() not in message:
                print("Current song order:")
                message = communicationSocket.recv(1024)
                message = message.replace(r'\n', '\n')
                print(message)
                print("Write your song order")
                songOrder = input()
                sendMessage(communicationSocket, songOrder.encode())
                message = communicationSocket.recv(1024)
                print(message)
                print(len(message))
                while "Wrong input values".encode() in message:
                    print('trying again..')
                    songOrderRetry = input()
                    sendMessage(communicationSocket, songOrderRetry.encode())
                    message = communicationSocket.recv(1024)
                    print(message)
                print("success")

        # if a == 'edit':
        #     #prosba o edycje


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
