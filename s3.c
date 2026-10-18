#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char *args[], int argsc)
{
    execvp(args[ARG_PROGNAME], args); // execvp the child to a different program

    perror("execvp failed"); // only reach here if execvp fails
    exit(1); // exit child with error
}

void launch_program(char *args[], int argsc)
{
    if (argsc == 0) { // no command
        perror("no command entered");
        exit(1);
    }

    if (strcmp(args[ARG_PROGNAME], "exit") == 0) { // handle exit command BEFORE forking so shell closes not just child
        exit(0);
    }

    int rc = fork(); // fork a child (return code)

    if (rc < 0) { // fork failed
        perror("child process fork failed");
        exit(1);
    } else if (rc == 0) { // child process
        child(args, argsc);
    } else { // parent process
        int wc = wait(NULL); // wait for child to finish (wait code)
        if (wc == -1) { // wait failed
            perror("wait failed");
            exit(1);
        } // else positive wc gives child process completion
    }
}