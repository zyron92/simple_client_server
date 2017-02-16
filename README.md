# Simple Client & Server Application
A simple client and a server application

##
###Preliminaries
csapp module (csapp.c & csapp.h) is borrowed from the [page](http://csapp.cs.cmu.edu/public/code.html) of Bryant and O'Hallaron's Computer Systems: A Programmer's Perspective.

##
###Descriptions
A simple client and a server application (blocking I/O) which communicates with a custom protocol based on TCP/IP. 
The server waits a connection trial from the client with port 12345, and the client will ask the server to conduct several jobs.

Makefile will produce two executable programs (client and server). Client program will run several jobs automatically, while server program will wait commands from the client. Client program needs a file to be executed with, where this file will be sent to the server program via network. Client program will send firstly a hello command and then wait for a hello command from the server program. After that, client program will send the file to the server program (multiple data packets). Once sending is done, client will send a data store command in order for the server program to store all the received data packets in a file named server.out .

##
###Header Format (total = 8 bytes)<br>
|<-8bits->|<br>
+-------------+-------------+-----------+-----------+<br>
| &nbsp; version &nbsp; | &nbsp; user ID &nbsp; | &nbsp; sequence num &nbsp;|<br>
+-------------+-------------+-----------+-----------+<br>
| &nbsp; total packet length &nbsp; | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; command &nbsp;&nbsp;&nbsp;&nbsp; |<br>
+-------------+-------------+-----------+-----------+<br>

##
###Command: client command for the server job
- 0x1 - hello (client hello)
- 0x2 - hello (server hello)
- 0x3 - data delivery (each sent data packet with a sequence number)
- 0x4 - data store (save all received data packets to a file)
- 0x5 - error

##
There are still bugs to be resolved, and improvements to be done.
