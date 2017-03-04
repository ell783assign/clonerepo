<center>

## **<u><font color="#006600">Assignment 1</font></u>**

</center>

<center>

## <font color="#CC0000">Topic: FTP Application  

</font><font color="#000000">Due on or before:</font> <font color="#3333FF">11 March, 2017 (Saturday)</font>

</center>

<center>

## <font color="#000000">Maximum Marks: 5</font>

</center>

<center>

* * *

</center>

Implement an ftp server with multiple clients using sockets. A client should be able to access the server, choose a file and transfer it to/from the server. A client should be able to run at least the following commands:

*   `ls`
*   `cd`
*   `chmod`
*   `lls` (local `ls` on the client)
*   `lcd` (local `cd` on the client)
*   `lchmod` (local `chmod` on the client)
*   `put`
*   `get`
*   `close`

* * *

## <font color="#3333FF">Take-Home Message</font>

Sockets are perhaps the best-known and most flexible mechanism for inter-process communication. The assignment aims to make one comfortable with TCP/IP sockets across different computers. `ftp` is a complex protocol to transfer files across a network. The assignment seeks to get a bare-bones implementation of the main part of the file transfer, without bothering about connections, authentication, encryption and access issues.

* * *
Let's start with some outline:

## Let us just break down the design of these programs into codable chunks.

The assignment consists of two main parts: An FTP **Server** and an FTP **Client**.

### FTP Server Components:

* Open Socket to listen for incoming connections.
* Receive messages on the socket.
* Parse incoming message as per FTP protocol interface specifications.
* Command interpretation layer: `cd`, `ls`, `chmod`, `put`, `get`, `close` received in Network packet.
* Implement methods for `cd`, `ls`, `chmod`, `save_resource`, `open_resource` (for `cd`, `ls`, `chmod`, `put` request and `get` request).

### FTP Client Components:

* Routines to Open Socket to send data on.
* Create FTP Packets and send on socket.
* Command Line interface implementing interpretation of commands from user for: `cd`, `lcd`, `ls`, `lls`, `chmod`, `lchmod`, `put`, `get`, `close`.
* Implement commands: `cd`, `ls`, `chmod`, `save_resource`, `open_resource` locally (for `lcd`, `lls`, `lchmod`, `get` response and `put`request).
* Implement FTP Network Messages for `cd`, `ls`, `chmod` and others.

### Common Routines for both:

* Network Message parsing.
* Opening local resources to send.
* Saving received data into a local resource.
* `cd`, `chmod` and `ls` are symmetrical.

### Comments:

- For starters, we can skip the beautification features like showing the progress of upload/download.
- Let us try to get to a working barebones version as soon as possible. Divide up work, define the interfaces over which various components work.
  A possible scenario is to first break the problem statement into sub-programs - 1 for each:

  * Write a program to take user input to implement `cd`, `ls`, `chmod`.
  * Write a program to open a file and make a copy of that file byte-by-byte into a different file (this implements first steps of `get` and `put`)
  * Write a program to communicate messages over a TCP/IP and/or UDP socket.
