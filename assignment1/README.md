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
