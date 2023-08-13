/**
 * @file writer.c
 * @brief Trabajo practico 1. Sistemas Operativos de Proposito General.
 * @author Gonzalo G. Fernandez
 * @note
 * - If a signal is received before getting a reader, the program exits with error.
 *
 */

#include <errno.h>    // errno, error code names
#include <fcntl.h>    // open
#include <signal.h>   // sigaction
#include <stdbool.h>  // bool type
#include <stdio.h>    // printf
#include <string.h>   // strlen, strcpy
#include <sys/stat.h> // mknod
#include <unistd.h>   // write, getpid

#include "utils.h"

int fd;                 /*!> File descriptor for PIPE */
bool open_pipe = false; /*!> Flag to indicate open PIPE */

/**
 * @brief Signal handler
 * @param signo: Number of signal received
 */
void signal_handler(int signo) {
    // Chack if open PIPE
    if (!open_pipe)
        return;

    switch (signo) {
    case SIGUSR1:
        write(fd, sigusr1_msg, MSG_SIGUSR_LEN);
        break;
    case SIGUSR2:
        write(fd, sigusr2_msg, MSG_SIGUSR_LEN);
    default:
        break;
    }
}

int main(void) {
    char buffer[BUFFER_SIZE];
    int return_code;
    ssize_t bytes_wrote;
    unsigned int prefix_len = strlen(data_msg_prefix);

    printf("Writer process initializaton. PID %d\n", getpid());

    // Create tmp folder with permissions 6:110 rw-
    return_code = mkdir(pipe_dir_path, 0777);
    if (return_code == -1 && errno != EEXIST) {
        perror("Error creating PIPE path directory");
        return 1; // Exit with error
    }

    // Named pipe permissions 6:110 rw-
    return_code = mknod(pipe_name, S_IFIFO | 0666, 0);

    if (return_code == EEXIST) // PIPE file already exists
    {
        perror("Error creating named pipe");
    } else if (return_code < -1) {
        perror("Error creating named pipe");
        return 1; // Exit with error
    }

    /* Signal handle */
    struct sigaction hsiguser1;
    struct sigaction hsiguser2;

    hsiguser1.sa_handler = signal_handler;
    hsiguser1.sa_flags = SA_RESTART;
    hsiguser2.sa_handler = signal_handler;
    hsiguser2.sa_flags = SA_RESTART;

    sigemptyset(&hsiguser1.sa_mask);
    sigemptyset(&hsiguser2.sa_mask);
    sigaction(SIGUSR1, &hsiguser1, NULL);
    sigaction(SIGUSR2, &hsiguser2, NULL);

    /* Open named pipe. Blocks until reader is found */
    // TODO: A timeout can be implemented
    printf("Waiting for reader...\n");
    if ((fd = open(pipe_name, O_WRONLY)) < 0) {
        perror("Error opening named pipe");
        return 1;
    }
    /**
     * Open syscalls returned without error,
     * meaning other process is attached to named pipe in read only mode
     */
    printf("Got a reader. Type some stuff:\n");
    open_pipe = true;

    /* Writer loop */
    while (1) {
        /* Add buffer prefix */
        strcpy(buffer, data_msg_prefix);
        /* Get text from stdin (console) */
        if (fgets(buffer + prefix_len, BUFFER_SIZE - prefix_len, stdin) == NULL) {
            /* Handle errors with stdin capture */
            if (errno == EINTR) {
                /* The system call can be interrupted by SIGUSR1 and SIGUSR2 signals */
                perror("Interupted stdin capture");
                continue;
            }
            perror("Error with string from stdin");
            return 1;
        }

        /* Write buffer to named pipe */
        if ((bytes_wrote = write(fd, buffer, strlen(buffer))) < 0) {
            perror("Error writing named pipe");
            continue;
        }
        printf("Wrote %ld bytes: %s", bytes_wrote, buffer);
    }

    return 0;
}
