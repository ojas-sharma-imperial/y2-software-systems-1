#include "s3.h"

int main(int argc, char *argv[]){
    char line[MAX_LINE]; // Stores the command line input

    while (1) {
        read_command_line(line);
        launch_program_master(line);
    }

    return 0;
    
}