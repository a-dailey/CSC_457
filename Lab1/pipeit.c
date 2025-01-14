#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


//#include "pipeit.h" 

#define STDIN 0
#define STDOUT 1

int main(int argc, char **argv) {
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(1);
    }

    pid_t pid1, pid2;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        printf("pipe creation failed\n");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();

    if (pid1 == -1) {
        printf("fork 1 failed\n");
        exit(EXIT_FAILURE);
    }


    if (pid1 == 0)
    {
        //child 1
        close(pipefd[0]); //close read side

        dup2(pipefd[1], STDOUT); //redirect stdout to pipe

        close(pipefd[1]); //close write side
        
        execlp("ls", "ls", NULL, NULL);
        printf("execlp failed\n");

        //close(pipefd[1]); //close write side
        exit(EXIT_FAILURE);
    }

    
    //PID1 parent
    pid2 = fork();

    if (pid2 == -1) {
        printf("fork 2 failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        //child 2
        close(pipefd[1]); //close write side
        dup2(pipefd[0], STDIN); //replace stdin with pipe
            
        //write only, create if DNE, clear if exists, set permissions for file
        //(Owner: rw, Group: r, Other: r)
        int outfile = open("outfile", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (outfile == -1) {
            printf("open failed\n");
            exit(EXIT_FAILURE);
        }
        

        dup2(outfile, STDOUT); //redirect stdout to outfile

        close(pipefd[0]); //close read side

        execlp("sort", "sort", "-r", NULL);
        printf("execlp failed\n");
        exit(EXIT_FAILURE);
    }

    else {
        //PID2 parent
        close(pipefd[0]);
        close(pipefd[1]);

        waitpid(pid1, NULL, 0); //wait for child 1 to finish
        waitpid(pid2, NULL, 0); //wait for child 2 to finish
        exit(EXIT_SUCCESS);

    }
    
        
    
    


}