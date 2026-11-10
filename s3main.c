#include "s3.h"

int main(int argc, char *argv[]) {
    char line[MAX_LINE]; // store the command line input
    while (1) {
        if ((argc == 3) && (strcmp(argv[ARG_1], "-s") == 0)) { // running a subshell process
            strcpy(line, argv[ARG_2]); // copy arg to line to process
            launch_program_master(line);
            return 0; // exit once subshell has completed
        } else { // normal shell process on start, main parent
            read_command_line(line); // read cli input into line
            launch_program_master(line);
        }
    }
}