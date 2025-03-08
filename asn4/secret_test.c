#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define SECRET_FILE "/dev/Secret"
#define SSGRANT _IOW('K', 1, uid_t)

int main(int argc, char *argv[]) {
    int fd, res;
    uid_t new_owner;
    char *secret = "This is a test secret";

    /* Open /dev/Secret for writing */
    fd = open(SECRET_FILE, O_WRONLY);
    if (fd < 0) {
        perror("Error opening " SECRET_FILE);
        return EXIT_FAILURE;
    }
    printf("Opened %s successfully (fd=%d)\n", SECRET_FILE, fd);

    /* Write a secret to the device*/
    res = write(fd, secret, strlen(secret));
    if (res < 0) {
        perror("Error writing to " SECRET_FILE);
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Secret written successfully (res=%d)\n", res);

    /* Check if a user ID is provided for transfer*/
    if (argc > 1) {
        new_owner = atoi(argv[1]);
        if (new_owner == 0) {
            fprintf(stderr, "Invalid user ID.\n");
            close(fd);
            return EXIT_FAILURE;
        }

        /*Perform ioctl to transfer ownership*/
        res = ioctl(fd, SSGRANT, &new_owner);
        if (res < 0) {
            perror("Error granting ownership");
        } else {
            printf("Ownership transferred to user ID %d successfully.\n",
                 new_owner);
        }
    } else {
        printf("No new owner provided. Secret remains with the 
            original owner.\n");
    }

    /*Close the file descriptor*/
    res = close(fd);
    if (res < 0) {
        perror("Error closing file descriptor");
        return EXIT_FAILURE;
    }
    printf("File descriptor closed successfully.\n");

    return EXIT_SUCCESS;
}
