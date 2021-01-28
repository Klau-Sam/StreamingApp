import socket  # Import socket module
import pyaudio
import sys
import time
from _thread import *
import os
import threading
import time

def createStream(Music):
    stream = Music.open(
        format=8,
        channels=2,
        rate=44100,
        output=True
    )
    return stream


def connect2(s, port):
    host = "192.168.1.23"
    s.connect((host, port))
    print("connected")
def playmusic(stream):
    global playing
    print("simea")
    time.sleep(2)
    while playing !='':

        #print("simea")

        stream.write(playing)
        print(playing)

def callback(wf,in_data, frame_count, time_info, status):
    data = wf.readframes(frame_count)
    return (data, pyaudio.paContinue)

def run2(s):

    data = s.recv(32)
    music = pyaudio.PyAudio()

    stream = music.open(
        format=8,
        channels=2,
        rate=44100,
        output=True
    )

    while len(data) > 0:
        stream.write(data)
        data = s.recv(32)

    # wait for stream to finish (5)

    stream.stop_stream()
    stream.close()


    # close PyAudio (7)
    music.terminate()
def run(s, stream):

    global playing
    playing =  s.recv(512)
    start_new_thread(playmusic,(stream,))

    data = s.recv(512)
    while data != '':


        playing= playing+s.recv(512)


        #print(playing)

    stream.close()
# def run(s, stream):
#     data = s.recv(16)
#     # print(data)
#     #playing = data
#
#     while data != '':
#         stream.write(data)
#
#         try:
#             data = s.recv(16)
#         except:
#             print('stopped playing')
#             break
#     stream.close()

def sendFile(sock, songname):
    filename = songname  # In the same folder or path is this file running must the file you want to tranfser to be
    f = open(filename, 'rb')

    f.seek(0, os.SEEK_END)
    print("Size of file is :", f.tell(), "bytes")
    size=str(f.tell())

    sock.send(size.encode())
    f.seek(0,0)
    time.sleep(2)
    l = f.read(32)
    #file_stats = os.stat(f)
    current_size = f.tell()
    print('rozmiar pliku',current_size)

    # print(l)
    # size=(sys.getsizeof(l))
    # print(size)
    # print(size.to_bytes(8, byteorder='big'))
    print("sending, please wait a few seconds")
    while (l):
        sock.send(l)
        # print('Sent ', repr(l))
        l = f.read(32)
    f.close()


def sendMessage(sock, message):
    sock.send(message)


# def poll(sock):
#     #mess = sock.recv(1024)
#     print('siema')
#     while  not exit_event.is_set():
#         True
#     sock.setblocking(0)
#     print('event set')
#     while exit_event.is_set():
#
#         mess = str(sock.recv(1024))
#         if len(mess) > 0:
#             print(mess)
#             ans = input()
#             while not ans == "Y" or ans == "N":
#                 print("wrong message! type Y if you agree or N if you dont.")
#             else:
#                 break
#     sock.setblocking(1)
#     sock.send(ans)
#     return 0


def main():
    silence = True
    communicationSocket = socket.socket()  # creates socket
    connect2(communicationSocket, 27)  # connects socket via hostname and port from input arguments
    # start_new_thread(poll, (communicationSocket,))
    init = True
    while True:

        if init == True:
            message = communicationSocket.recv(1024)
            message = str(message)
            message = message.replace(r'\n', '\n')
            print(message)
            init = False

        a = input()
        # if a == "poll":
        #
        #     exit_event.set()
        #
        #     print(threading.active_count())
        #     a = input()
        #     exit_event.clear()

        if a == 'send':
            print('you want to upload new song.')
            sendMessage(communicationSocket, 'send'.encode())

            message = communicationSocket.recv(1024)
            print(message)
            print(
                "Write a name of your song(only .wav format, make sure its in your folder and to include .wav at the end")
            songname = input()
            sendMessage(communicationSocket, songname.encode())
            message = communicationSocket.recv(1024)
            print(message)


            while "That's the ".encode() in message:
                songname = input()
                sendMessage(communicationSocket, songname.encode())
                message = communicationSocket.recv(1024)
                print(message)
            if "Song already exists".encode() in message:
                True
            else:
                sendFile(communicationSocket, songname)

                #sendMessage(communicationSocket, "end".encode())
                print("song sent.")
        if a == 'music':

            print('playing')
            if silence:
                playSocket = socket.socket()  # creates socket
                connect2(playSocket, 14)
                # Music = pyaudio.PyAudio()  # initiate pyaudio variable
                # stream = createStream(Music)
                start_new_thread(run2, (playSocket,))
                print("out of thread")
                silence = False
            else:
                playSocket.close()
                silence = True

        if a == 'skip':
            print('you want to skip some songs.')
            sendMessage(communicationSocket, 'skip'.encode())
            message = communicationSocket.recv(1024)
            print(message)
            if "Sorry".encode() not in message:
                print("Current song order:")
                message = communicationSocket.recv(1024)
                message=str(message)
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
            return 0

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
                message = str(message)
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
