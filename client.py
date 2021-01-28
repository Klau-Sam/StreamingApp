import socket
import pyaudio
import sys
import time
from _thread import *
import os
import threading
import time
import select
if len(sys.argv) != 4:
    raise ValueError('Please provide server ip and two ports')


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
        try:
            data = s.recv(32)
        except:
            print('stopped playing')
            break

    stream.stop_stream()
    stream.close()

def sendFile(sock, songname):
    filename = songname  # In the same folder or path is this file running must the file you want to tranfser to be
    f = open(filename, 'rb')
    f.seek(0, os.SEEK_END)
    print("Size of file is :", f.tell(), "bytes")
    size = str(f.tell())
    sock.send(size.encode())
    f.seek(0, 0)
    time.sleep(2)
    l = f.read(32)
    print("sending, please wait a few seconds")
    while (l):
        sock.send(l)
        l = f.read(32)
    f.close()

def sendMessage(sock, message):
    try:

        sock.send(message)
    except ConnectionAbortedError:
        print("server shutdown. closing client program")
        sys.exit(-1)

# def poll(sock):           #nieudana próba stworzenia ankiety
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

def checkConnection(socket,host,port):
    while True:
        try:
            socket.fileno()>0
        except:
            print("server disconnected.")
            sys.exit(-1)

def main():
    host = sys.argv[1]
    port = int(sys.argv[2])
    silence = True
    communicationSocket = socket.socket()  # creates socket
    try:
        communicationSocket.connect((host, port))
    except:
        print("socket timeout. Server not responding. Make sure server is online and you have provided correct arguments")
        sys.exit(-1)
    # start_new_thread(poll, (communicationSocket,))
    #start_new_thread(checkConnection, (communicationSocket,host,port))
    init = True
    while True:

        if init == True:
            message = communicationSocket.recv(1024)
            message = str(message)
            message = message.replace(r'\n', '\n')
            print(message)
            print("Avaiable commands:\n"
                  "'music' to turn music on and off\n"
                  "'send' to upload a new song\n"
                  "'skip' to skip to another song in the playlist \n"
                  "'list' to display current playlist\n"
                  "'edit' to change order of the playlist\n"
                  "'quit' to end the program.")
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
            sendMessage(communicationSocket, 'send'.encode())
            message = communicationSocket.recv(1024)
            print(message)
            songname = input()
            sendMessage(communicationSocket, songname.encode())
            message = communicationSocket.recv(1024)
            print(message)
            while "That's the ".encode() in message:
                songname = input()
                sendMessage(communicationSocket, songname.encode())
                if songname=='exit':
                    print('exiting')
                    break
                else:
                    message = communicationSocket.recv(1024)
                    print(message)

            if "Song already exists".encode() in message:
                True
            else:
                if not songname=="exit":
                    while True:
                        try:
                            sendFile(communicationSocket, songname)
                            break
                        except:
                            print("no such song in your directory. Try again")
                            songname=input()

                    print("song sent.")
            print('cya')
        if a == 'music':
            print("playing music. write 'music' again to stop.")
            if silence:
                port2 = (int(sys.argv[3]))
                playSocket = socket.socket()  # creates socket for music streaming
                try:
                    playSocket.connect((host, port2))
                except:
                    print(
                        "music socket timeout. Make sure server is online and you have provided correct arguments")
                    return -1
                start_new_thread(run2, (playSocket,))
                silence = False
            else:
                silence = True
                playSocket.close()
        if a == 'skip':
            sendMessage(communicationSocket, 'skip'.encode())
            message = communicationSocket.recv(1024)
            print(message)
            if "Sorry".encode() not in message:
                print("Current song order:")
                message = communicationSocket.recv(1024)
                message = str(message)
                message = message.replace(r'\n', '\n')
                print(message)
                songSkip = input()
                sendMessage(communicationSocket, songSkip.encode())
                message = communicationSocket.recv(1024)
                print(message)
                while "Wrong input value".encode() in message:
                    songSkip = input()
                    sendMessage(communicationSocket, songSkip.encode())
                    if songSkip == 'exit':
                        print('exiting')
                        break

                    else:
                        message = communicationSocket.recv(1024)
                        print(message)


        if a == 'list':
            print('next songs in the playlist:')
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
                    if songOrderRetry == 'exit':
                        print('exiting')
                        break
                    else:
                        message = communicationSocket.recv(1024)
                        print(message)
                print("success")
        if a == 'quit':
            sendMessage(communicationSocket, 'quit'.encode())
            print("good bye!")
            sys.exit(0)

if __name__ == "__main__":
    main()
