#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

//#include "pipeit.h"


int main(int argc, char *argv) {
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(1);
    }

    pid_t pid1, pid2;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid1 = fork();

    if (pid1 == -1) {
        perror("fork");
        exit(1);
    }

    if (pid1 = 0)
    {
        //child 1
        close(pipefd[0]); //close read side

        FILE *file = popen("ls", "r");

        if (file == NULL) {
            perror("fopen");
            exit(1);
        }

        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file) != NULL) { //read from LS call
            write(pipefd[1], buffer, strlen(buffer)); //write to pipe
        }

        close(pipefd[1]); //close write side
        pclose(file);
        exit(0);
    }

    if (pid1 > 0) {
        pid2 = fork();

        if (pid2 == -1) {
            perror("fork");
            exit(1);
        }

        if (pid2 == 0) {
            //child 2
            close(pipefd[1]); //close write side

            char buffer[1024];
            while (read(pipefd[0], buffer, sizeof(buffer)) > 0) { //read from pipe
                printf("%s", buffer); //print to stdout
            }

            close(pipefd[0]); //close read side
            exit(0);
        }
    }
    


}