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
