## Problem Statement

__SHELL UTILITY__

- Using system calls to handle all file operations, write a program in C/C++/Java that copies a file from one location to another, and deletes the original version.

- The program should take two file names as its arguments, the first one being the file to copy, and the second being the new location.

- The new location should not be over-written if it already exists. 
- The program should check for bad inputs and should work for files of arbitrary size.
- The program should gracefully handle error conditions while copying the file.
- Make sure to use system calls (e.g., open) and not the C library for manipulating files (e.g., fopen).

The Simple Shell: 

The behaviour of the simple shell is simply an infinite loop:

- Displaying a prompt to indicate that it is ready to accept the next command from the user
- Reading a line of keyboard input as a command, and
- Spawning and having a new process execute as per the user's command
- The shell should read a line from the standard input, parse the line to get the command and its arguments, fork and exec the command. 
- The shell should wait for commands to finish
- The shell should exit when it receives the command exit.
- When executing a command, the shell should first look for the command in the current directory, and if not found, search the directories defined in a special variable, path.
- Some general-purpose commands such as ls or installed software have their executables stores in folder /usr/bin. The shell should not be able to run these commands, and report an error till /usr/bin has been added to the path.
- Using the system command is not allowed, as it simply invokes the system's /bin/sh shell to do the work.
- You may assume that the command line arguments are separated by whitespaces. Do not do anything special for backslashes, quotes, ampersands or other characters that are ``special'' in other shells. Note that this means that commands in your shell will not be able to operate on filenames with spaces in them!
- You may set a reasonable maximum on the number of command line arguments, but the shell should handle input lines of any length.
- The executable file for your shell should be named sh. When executing it, make sure you are executing your own sh and not /bin/sh.

- Implement cd, the change directory commands. You will need to invoke the chdir system call to do this. Note that if the call to chdir fails, you probably don't want to exit the shell, but instead, should handle the error appropriately.
- Some convenient built-ins present in many shells are pushd, popd and dirs. Implement these in your shell. Do not assume any limit on the number of commands that may be executed.
- pushd works like cd, but pushes the directory you switched from onto a stack
- popd pops the top directory off the stack, and cds to it. In other words, a pushd followed by a popd should bring you back to the directory you were in, before the pushd.
- dirs prints the contents of the stack.

- The path variable holds a list of possible paths in which to search for executables. The list of paths is empty by default, but may grow to any arbitrary size. You should implement a built-in command to control this variable: path [+|- /some/dir]
- path (without arguments) displays all entries in the list separated by colons e.g., /bin:/usr/bin.
- path + /some/dir appends the given pathname to the path list.
- path - /some/dir removes the given pathname from the path list.
