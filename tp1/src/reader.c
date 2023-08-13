/**
 * @file reader.c
 * @brief Trabajo practico 1. Sistemas Operativos de Proposito General.
 * @author Gonzalo G. Fernandez
 * @note
 *
 */

#include <errno.h>    // errno, error code names
#include <fcntl.h>    // open
#include <signal.h>   // sigaction
#include <stdio.h>    // printf
#include <string.h>   // strlen, strcpy, strstr
#include <sys/stat.h> // mknod
#include <unistd.h>   // write

#include "utils.h"

FILE *pdata_log; /*!> Data logging file pointer */
FILE *psign_log; /*!> Data logging file pointer */

/**
 * @brief Signal handler
 * @param signo: Number of signal received
 */
void signal_handler(int signo) {
    if (signo == SIGINT) {
        // Close file pointers
        fclose(pdata_log);
        fclose(psign_log);
    }
}

int main(void) {
    char buffer[BUFFER_SIZE];
    char *buffer_content = (buffer + MSG_PREFIX_LEN); // Pointer to message without prefix
    int return_code, fd;
    ssize_t bytes_read;
    FILE *plog;

    printf("Reader process initializaton. PID %d\n", getpid());

    // Create tmp folder with permissions 6:110 rw-
    return_code = mkdir(pipe_dir_path, 0777);
    if (return_code == -1 && errno != EEXIST) {
        perror("Error creating tmp directory");
        return 1; // Exit with error
    }

    // Create log folder with permissions 6:110 rw-
    return_code = mkdir(log_dir_path, 0777);
    if (return_code == -1 && errno != EEXIST) {
        perror("Error creating log directory");
        return 1; // Exit with error
    }

    /* Logging files setup */
    pdata_log = fopen(data_log_path, "a"); // Open data log file in append mode
    psign_log = fopen(sign_log_path, "a"); // Open signals log file in append mode

    if (pdata_log == NULL || psign_log == NULL) {
        printf("Error open logging files.\n");
        return 1;
    }

    // Named pipe permissions 6:110 rw-
    return_code = mknod(pipe_name, S_IFIFO | 0666, 0);

    if (return_code == -1) {
        perror("Error creating named pipe");
    } else if (return_code < -1) {
        perror("Error creating named pipe");
        return 1; // Exit with error
    }

    /* Signal handle */
    struct sigaction hsigint;
    hsigint.sa_handler = signal_handler;
    hsigint.sa_flags = 0;
    sigemptyset(&hsigint.sa_mask);
    sigaction(SIGINT, &hsigint, NULL);

    /* Open named pipe. Blocks until reader is found */
    // TODO: A timeout can be implemented
    printf("Waiting for writer...\n");
    if ((fd = open(pipe_name, O_RDONLY)) < 0) {
        perror("Error opening named pipe");
        return 1;
    }

    /**
     * Open syscalls returned without error,
     * meaning other process is attached to named pipe in read only mode
     */
    printf("Got a writer\n");

    size_t write_chk; // Aux var to check bytes written

    /* Reader loop */
    do {
        /* Read named pipe into local buffer */
        if ((bytes_read = read(fd, buffer, BUFFER_SIZE)) < 0) {
            perror("Error reading named pipe");
            continue;
        }

        buffer[bytes_read] = '\0';
        printf("Read %ld bytes: %s", bytes_read, buffer);

        if (NULL != strstr(buffer, data_msg_prefix)) {
            plog = pdata_log;
        } else if (NULL != strstr(buffer, sign_msg_prefix)) {
            plog = psign_log;
        } else {
            continue;
        }
        write_chk = fwrite(buffer_content, sizeof(char), strlen(buffer_content), plog);
        if (write_chk < strlen(buffer_content)) {
            perror("Error encountered when writing log file");
        }

    } while (bytes_read > 0);

    // Close file pointers
    if (fclose(pdata_log) != 0 || fclose(psign_log) != 0) {
        perror("Error closing log file");
    }

    return 0;
}
