#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

int command_with_redirection(char line[]) { // check whether command has redirection; return 1 if true else 0
    int inout = 0; // 1 for output redirected and 2 for input redirected; 0 for no redirection
    char* input = strchr(line, '<');
    char* output = strchr(line, '>');

    if ((input == NULL) && (output == NULL)) {
        return 0; // no redirection
    } 
    if (input != NULL) {
        inout += 2;
    }
    if (output != NULL) {
        inout += 1;
    }
    return inout;
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

void parse_command_with_redirection(char line[], char *args[], int *argsc, char **input_file, char **output_file, int *append_output)
{
    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    *input_file = NULL;
    *output_file = NULL;

    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        if ((strcmp(token, ">") == 0) || (strcmp(token, ">>") == 0)) { // output redirection
            if (strcmp(token, ">>") == 0) { // check for whether appending
                *append_output = 1;
            } else {
                *append_output = 0;
            }
            token = strtok(NULL, " "); // get next token
            if (token == NULL) {
                perror("missing redirection filename");
                exit(1);
            }
            *output_file = token;
        } else if ((strcmp(token, "<") == 0)) { // input redirection
            token = strtok(NULL, " "); // get next token
            if (token == NULL) {
                perror("missing redirection filename");
                exit(1);
            }
            *input_file = token;
        } else { // no redirection at this point, advance and add to command args
            args[(*argsc)++] = token;
        }
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

void launch_program_with_redirection(char *args[], int argsc, int redirection, char **input_file, char **output_file, int *append_output) {
    if (argsc == 0) { // no command (shouldn't be possible to reach this)
        perror("no command entered");
        exit(1);
    }

    if (strcmp(args[ARG_PROGNAME], "exit") == 0) { // handle exit command BEFORE forking so shell closes not just child (shouldn't be possible to reach this)
        exit(0);
    }

    if (redirection == 1) { // output redirection
        int fd; // (file descriptor)
        if (*append_output) { // append to file
            fd = open(*output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else { // overwrite file
            fd = open(*output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        if (fd == -1) { // catch unopened file errors
            perror("unable to open file");
            exit(1);
        }
        int rc = fork(); // fork a child (return code)
        if (rc < 0) { // fork failed
            perror("child process fork failed");
            exit(1);
        } else if (rc == 0) { // child process
            child_with_redirected_output(args, argsc, fd);
        } else { // parent process
            int wc = wait(NULL); // wait for child to finish (wait code)
            if (wc == -1) { // wait failed
                perror("wait failed");
                exit(1);
            } // else positive wc gives child process completion
        }
    } else if (redirection == 2) { // input redirection
        int fd; // (file descriptor)
        fd = open(*input_file, O_RDONLY);
        if (fd == -1) { // catch unopened file errors
            perror("unable to open file");
            exit(1);
        }
        int rc = fork(); // fork a child (return code)
        if (rc < 0) { // fork failed
            perror("child process fork failed");
            exit(1);
        } else if (rc == 0) { // child process
            child_with_redirected_input(args, argsc, fd);
        } else { // parent process
            int wc = wait(NULL); // wait for child to finish (wait code)
            if (wc == -1) { // wait failed
                perror("wait failed");
                exit(1);
            } // else positive wc gives child process completion
        }
    } else if (redirection == 3) { // input and output redirection
        perror("input and output redirected");
        exit(1);
    } else {
        perror("unable to understand redirection");
        exit(1);
    }
}

void child_with_redirected_input(char *args[], int argsc, int fd) {
    if (dup2(fd, STDIN_FILENO) == -1) { // link std output to file using dup2
            perror("dup2 failed");
            close(fd);
            exit(1);
    }
    close(fd);
    execvp(args[ARG_PROGNAME], args); // execvp the child to a different program
    perror("execvp failed"); // only reach here if execvp fails
    exit(1); // exit child with error
}

void child_with_redirected_output(char *args[], int argsc, int fd) {
    if (dup2(fd, STDOUT_FILENO) == -1) { // link std output to file using dup2
            perror("dup2 failed");
            close(fd);
            exit(1);
    }
    close(fd);
    execvp(args[ARG_PROGNAME], args); // execvp the child to a different program
    perror("execvp failed"); // only reach here if execvp fails
    exit(1); // exit child with error
}