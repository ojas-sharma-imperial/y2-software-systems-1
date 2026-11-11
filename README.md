# Software Systems Shell ($s^3$)

## Overview

This GitHub repository contains the code for a UNIX shell implemented in C. This README file contains the general strategy behind the tasks and the features implemented.

## Features

### Basic Commands

The shell constructs a string of the prompt line and parses through it to separate by whitespaces. The parent process (shell) then forks a child process that transforms to the desired command using ```execvp()```. Any additional arguements are passed to the child and are used by that executable.

### Redirection

In order to be able to redirect standard data in and data out streams, the redirection operatores were implemented, using file ```open()``` calls and ```dup2()``` system calls. The software was extended from the specification by allowing for both input and output redirection in the same command.

### Support for ```cd```

The ```cd``` command is not a native binary that can be run as a child process and must be handled by the shell itself. A specific function ```handle_cd()``` was implemented, using the ```chdir()``` system call to move the current directory of the program. The function must also remember the previous directory each time to handle the ```cd -``` command.

### Commands with Pipes

Pipes allow one command process to send data to another process, and are implemented using the ```pipe()``` system call. This required some extra effort during parsing as these can be chained, and hence the input end of a pipe must be correctly chained to the output end for the subsequent process. Fundamentally, a pipe is created between two processes, and the non-relevant parts of the pipe closed, before a ```dup2()``` system call is made to link to standard data streams.

Implementing this in the shell required new functions to be written to launch children (in this code called ```_master()```), as now the parent process must be able to launch multiple children and wait to reap all of them. This meant separating the ```launch_program()``` script from the parts where children were made. The `pid` of each child launched by a shell is stored in ```rcs[]``` (return codes) and is waited on by the parent shell process.

### Batched Commands

The semicolon acts as a delimiter to allow for completely independent commands in a shell. This functioned on the new `launch_program_master()` function, ensuring not to create a pipe, and closing off any previous pipes, since the delimiter being present means the end of a set of commands.

### Subshells

A subshell is a child process of the shell that itself runs a shell process. Since the subshells could be nested, the program utilises recursion to implement these. If there are no subshells in the current command, the shell executes it as normal.

If there are one or more subshells, the current shell extracts the largest subshell (of depth 1) and sends all of that as an argument to the next shell. It uses the linux file `/proc/self/exe` to launch itself as a child of itself, and uses the flag `-s` to indicate that this is a child shell, meaning it shouldn't look for user input, and should instead take the arguments given by its parent shell.

This strategy quite simply solves the problem of nested subshells as, if there were a depth 2 nested subshel, then the child subshell will just launch another child subshell of it, seeing this as a depth 1 subshell. This allows for minimal code and does not require the use of a stack to operate, merely a depth counter for parenthesis.