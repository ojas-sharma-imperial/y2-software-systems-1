#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    while (1) {

        read_command_line(line);

        int redirection = command_with_redirection(line);
        int append_output;
        char *input_file, *output_file;
        
        if(redirection){///Command with redirection
           parse_command_with_redirection(line, args, &argsc, &input_file, &output_file, &append_output);
           launch_program_with_redirection(args, argsc, redirection, &input_file, &output_file, &append_output);
           reap();
       }
       else ///Basic command
       {
           parse_command(line, args, &argsc);
           launch_program(args, argsc);
           reap();
       }
    }

    return 0;
    
}