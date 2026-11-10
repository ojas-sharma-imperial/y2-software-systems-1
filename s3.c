#include "s3.h"

// State Variables

char PREV_DIR[MAX_LINE]; // store the previous working directory (for cd)

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    char cwd[MAX_LINE];
    getcwd(cwd, sizeof(cwd)); // get current working directory

    strcpy(shell_prompt, "[");
    strcat(shell_prompt, cwd);
    strcat(shell_prompt, " s3]$ ");
}

void handle_cd(char *args[], int argsc) {
    char temp[MAX_LINE];
    if (args[ARG_1] == NULL) { // cd to HOME
        getcwd(PREV_DIR, sizeof(PREV_DIR)); // store the cwd BEFORE cd
        chdir(getenv("HOME"));
    } else if (strcmp(args[ARG_1], "-") == 0) { // cd to previous
        getcwd(temp, sizeof(temp));
        chdir(PREV_DIR);
        strcpy(PREV_DIR, temp);
    } else { // standard cd
        getcwd(PREV_DIR, sizeof(PREV_DIR)); // store the cwd BEFORE cd
        chdir(args[ARG_1]);
    }
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

    if (strcmp(args[ARG_PROGNAME], "cd") == 0) { // handle cd command instead of forking a child
        handle_cd(args, argsc);
        return;
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

    if (strcmp(args[ARG_PROGNAME], "cd") == 0) { // handle cd command instead of forking a child
        handle_cd(args, argsc);
        return;
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

void launch_program_master(char line[]) {
    char *commands[MAX_LINE]; // commands split by pipes
    char delimiters[MAX_LINE]; // for parser to understand pipes/semicolons/etc.
    int rcs[MAX_LINE]; // store rcs (return codes) of children split by pipes
    int num_commands = 1;

    commands[0] = line;
    for (char *p = line; *p != '\0'; p++) { // run through characters in line (until null)
        if ((*p == '|') || (*p == ';')) {
            delimiters[num_commands - 1] = *p; // store what type of delimiter it is
            *p = '\0'; // terminate that command
            commands[num_commands++] = p + 1; // next command starts after the pipe or semicolon
        }
    }
    delimiters[num_commands - 1] = ';'; // last command has no pipe etc.

    int prev_fd = -1;
    int previous_stage = 0;
    int current_stage = 0;

    for (int stage = 0; stage < num_commands; stage++) { // iterate through each piped or terminated command
        char* args[MAX_ARGS];
        int argsc = 0;
        int append_output = 0;
        char *input_file, *output_file;

        parse_command_with_redirection(commands[stage], args, &argsc, &input_file, &output_file, &append_output);
        if (argsc == 0) {
            perror("empty pipe stage");
            exit(1);
        }

        if (strcmp(args[ARG_PROGNAME], "exit") == 0) { // handle exit command BEFORE forking so shell closes not just child
        exit(0);
        }

        if (strcmp(args[ARG_PROGNAME], "cd") == 0) { // handle cd command instead of forking a child
            handle_cd(args, argsc);
            return;
        }

        if (delimiters[stage] == '|') {
            int fd[2] = {-1, -1};
            if ((stage < num_commands - 1) && (pipe(fd) < 0)) {
                perror("pipe failed");
                exit(1);
            }

            current_stage++;
            rcs[stage] = setup_child(args, argsc, prev_fd, fd[WRITE_END], fd[READ_END], input_file, output_file, &append_output);

            if (prev_fd != -1) { // "shift" the pipe stages and close off unnecessary fds in the parent
                close(prev_fd);
            }
            if (fd[WRITE_END] != -1) {
                close(fd[WRITE_END]);
            }
            prev_fd = fd[READ_END];

        } else if (delimiters[stage] == ';') {
            current_stage++;
            rcs[stage] = setup_child(args, argsc, prev_fd, -1, -1, input_file, output_file, &append_output);

            if (prev_fd != -1) { // parent close its copy of the read end of the pipe
                close(prev_fd);
                prev_fd = -1;
            }

            for (int stage = previous_stage; stage < current_stage; stage++) {
                int wc = waitpid(rcs[stage], NULL, 0); // wait for all child processes to finish
                if (wc < 0) {
                    perror("waitpid failed");
                    exit(1);
                }
            }
            previous_stage = current_stage;
        }
    }
}

int setup_child(char *args[], int argsc, int input_fd, int output_fd, int unused_read_fd, char *input_file, char *output_file, int *append_output) {
    int rc = fork(); // fork a child (return code)

    if (rc < 0) { // fork failed
        perror("child process fork failed");
        exit(1);
    } else if (rc == 0) { // child process
        if (unused_read_fd != -1) {
            close(unused_read_fd); // close unused read end of pipe
        }
        master_child(args, argsc, input_fd, output_fd, input_file, output_file, append_output);
    }
    return rc;
}

void master_child(char *args[], int argsc, int input_fd, int output_fd, char *input_file, char *output_file, int *append_output) {
    if (input_fd != -1) { // input pipe
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 failed on input from pipe");
            close(input_fd);
            exit(1);
        }
        close(input_fd);
    }
    if (output_fd != -1) { // output pipe
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 failed on output from pipe");
            close(output_fd);
            exit(1);
        }
        close(output_fd);
    }

    if ((input_file != NULL) || (output_file != NULL)) { // some redirection; handles case of both directions
        if (input_file != NULL) { // input redirection
            int fd;
            fd = open(input_file, O_RDONLY);
            if (dup2(fd, STDIN_FILENO) == -1) { // link std input to file using dup2
                perror("dup2 failed on input redirection");
                close(fd);
                exit(1);
            }
            close(fd);
        }
        if (output_file != NULL) { // output redirection
            int fd;
            if (*append_output) {
                fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644); // append to file
            } else {
                fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // overwrite file
            }
            if (dup2(fd, STDOUT_FILENO) == -1) { // link std input to file using dup2
                perror("dup2 failed on output redirection");
                close(fd);
                exit(1);
            }
            close(fd);
        }
    } else if ((input_file == NULL) && (output_file == NULL)) { // no redirection
        // nothing to do here
    } else { // weird redirection error
        perror("unable to understand redirection");
        exit(1);
    }

    execvp(args[ARG_PROGNAME], args);
    perror("child execvp failed");
    exit(1);
}

void child_with_redirected_input(char *args[], int argsc, int fd) {
    if (dup2(fd, STDIN_FILENO) == -1) { // link std input to file using dup2
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