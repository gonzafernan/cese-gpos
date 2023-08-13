/**
 * @brief Serial service
 * @author Gonzalo G. Fernandez
 *
 */

#include "SerialManager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <errno.h>      // errno
#include <netinet/in.h> // sockaddr_in
#include <pthread.h>    // pthread_create, pthread_join, pthread_mutex, pthread_sigmask
#include <signal.h>     // sigaction
#include <string.h>
#include <strings.h>    // bzero
#include <sys/socket.h> // socket, bind, listen, accept
#include <unistd.h>

#define SERIAL_PORT_BAUDRATE 115200        /*!> Serial service device baudrate */
#define SERIAL_MSG_LENGTH 12               /*!> Serial protocol message length */
#define SERIAL_SERVICE_SERVER_PORT 10000   /*!> TCP server port */
#define SERIAL_SERVICE_IP_ADDR "127.0.0.1" /*!> TCP server IP address */

bool serial_lock; /*!> Flag for serial connected */
bool client_lock; /*!> Flag for client connected */

int fd_conn; /*!> File descriptor for connection (to listen the client) */

pthread_mutex_t serial_mutex = PTHREAD_MUTEX_INITIALIZER;

// Functions definition
void *serial_server_connect(void *args);

/**
 * @brief Serial service exit process
 */
void serial_service_exit(int exit_code) {
    if (serial_lock) {
        serial_close();
        printf("Serial port closed\r\n");
    }
    if (client_lock) {
        close(fd_conn);
        printf("Client socket closed\r\n");
    }
    exit(exit_code);
}

/**
 * @brief Block signals used by serial service to start threads safely
 * @param mask_how SIG_BLOCK or SIG_UNBLOCK depending the behavior expected
 * @reval int
 */
int serial_mask_signal(int mask_how) {
    int rcode;
    sigset_t sigset;
    rcode = sigemptyset(&sigset);
    if (0 != rcode) {
        return rcode;
    }
    rcode = sigaddset(&sigset, SIGINT);
    if (0 != rcode) {
        return rcode;
    }
    rcode = sigaddset(&sigset, SIGTERM);
    if (0 != rcode) {
        return rcode;
    }
    return pthread_sigmask(mask_how, &sigset, NULL);
}

void sa_serial_handler(int signo) {
    if (SIGINT == signo) {
        printf("Serial service exit by SIGINT\r\n");
        serial_service_exit(EXIT_SUCCESS);
    } else if (SIGTERM == signo) {
        printf("Serial service exit by SIGTERM\r\n");
        serial_service_exit(EXIT_FAILURE);
    }
}

/**
 * @brief Thread to listen serial port
 * @note The thread starts with an open serial port
 */
void *serial_port_listen(void *args) {
    printf("Serial port polling\r\n");

    serial_lock = true;
    char rx_buffer[SERIAL_MSG_LENGTH];
    int read_size;

    while (1) {
        sleep(1);
        // Non-blocking serial read (mutex since it's a shared resource)
        pthread_mutex_lock(&serial_mutex);
        read_size = serial_receive(rx_buffer, SERIAL_MSG_LENGTH);
        pthread_mutex_unlock(&serial_mutex);

        if (0 > read_size)
            continue; // No reading
        if (0 == read_size) {
            printf("WARNING: Serial port closed\r\n");
            pthread_exit(NULL);
        }

        printf("Serial service egress: %s", rx_buffer);
        if (!client_lock)
            continue;

        // write: write to client (the accepted connection)
        if (0 > write(fd_conn, (void *)rx_buffer, SERIAL_MSG_LENGTH)) {
            perror("WARNING: Unable to send message to client");
        }
    }
}

/**
 * @brief TCP/IP server listen thread
 */
void *serial_server_listen(void *args) {
    printf("Serial service TCP/IP server setup\r\n");

    int rcode;     // to check return values
    int fd_socket; // server soket file descriptor (to accept new connection)

    socklen_t addr_len;            // store sockaddr structure size
    struct sockaddr_in serveraddr; // server address (_in internet)
    struct sockaddr_in clientaddr; // client address (_in internet)

    pthread_t conn_thread; // connection thread, read loop

    // socket: socket creation
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > fd_socket) {
        perror("ERROR: Unable to create TCP server socket");
        return 0;
    }

    // set server address structure
    bzero((void *)&serveraddr, sizeof(serveraddr)); // initialize with \0 the structure
    serveraddr.sin_family = AF_INET;                // intenet family
    // set port. htons: host byte order to network byte order
    serveraddr.sin_port = htons(SERIAL_SERVICE_SERVER_PORT);
    // set IP address. inet_pton: convert IPv4 and IPv6 addresses from text to binary form
    rcode = inet_pton(AF_INET, SERIAL_SERVICE_IP_ADDR, (void *)&serveraddr.sin_addr);
    if (0 == rcode) {
        printf("ERROR: Invalid network address for server\r\n");
        return 0;
    } else if (0 > rcode) {
        perror("ERROR: Unable to set server IP address");
        return 0;
    }

    // bind: port and IP assignment
    if (0 > bind(fd_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) {
        perror("ERROR: Unable to bind TCP server socket");
        close(fd_socket);
        return 0;
    }

    // listen: server ready to accept connections
    // backlog: given the application architecture,
    // the maximum length of the pending connections queue is one.
    if (0 > listen(fd_socket, 1)) {
        perror("ERROR: Socket unable to listen for connections");
        return 0;
    }

    // accept: pop pending connections from backlog (blocking)
    char ip_client[32];
    while (1) {
        addr_len = sizeof(struct sockaddr_in);
        fd_conn = accept(fd_socket, (struct sockaddr *)&clientaddr, &addr_len);
        if (0 > fd_conn) {
            perror("ERROR: Refused pending connection, unable to accept");
            continue; // is this correct?
        }

        // log msg on connection. inet_ntop: convert IPv4 and IPv6 addresses from binary to text
        // form
        if (NULL !=
            inet_ntop(AF_INET, (void *)&clientaddr.sin_addr, ip_client, sizeof(ip_client))) {
            printf("New connection with client IP %s\r\n", ip_client);
        }

        serial_server_connect(NULL);

        // Block if multi-client
        // rcode = pthread_create(&conn_thread, NULL, serial_server_connect, NULL);
        // if (0 != rcode)
        // {
        //     errno = rcode;
        //     perror("ERROR: Unable to create server thread");
        //     close(fd_conn);
        //     return (void*)-1;
        // }
    }
}

/**
 * @brief TCPIP server connection thread
 */
void *serial_server_connect(void *args) {
    printf("Serial service transmit over TCP/IP server\r\n");

    client_lock = true;
    char tx_buffer[SERIAL_MSG_LENGTH];
    ssize_t read_size;

    while (1) {
        // read: read from client (the accepted connection) (blocking)
        read_size = read(fd_conn, (void *)tx_buffer, SERIAL_MSG_LENGTH);
        if (0 > read_size) {
            perror("ERROR: reading from client");
            break;
        } else if (0 == read_size) {
            printf("Client disconnected\r\n");
            break;
        }
        printf("Serial service ingress: %s", tx_buffer);
        pthread_mutex_lock(&serial_mutex);
        serial_send(tx_buffer, SERIAL_MSG_LENGTH);
        pthread_mutex_unlock(&serial_mutex);
    }

    close(fd_conn);
    client_lock = false;
}

int main(void) {

    printf("Inicio Serial Service\r\n");

    int rcode;           // to check return values
    serial_lock = false; // init lock, emulator not connected
    client_lock = false; // init lock, client not connected

    // Setup signal management
    struct sigaction sa_serial;
    sa_serial.sa_handler = sa_serial_handler;
    sa_serial.sa_flags = 0;
    if (0 != sigemptyset(&sa_serial.sa_mask)) {
        perror("ERROR: Unable to create signal empty set");
        serial_service_exit(EXIT_FAILURE);
    }
    if (0 != sigaction(SIGINT, &sa_serial, NULL)) {
        perror("ERROR: Unable to set signal handler");
        serial_service_exit(EXIT_FAILURE);
    }
    if (0 != sigaction(SIGTERM, &sa_serial, NULL)) {
        perror("ERROR: Unable to set signal handler");
        serial_service_exit(EXIT_FAILURE);
    }

    rcode = serial_mask_signal(SIG_BLOCK);
    if (0 != rcode) {
        if (rcode != -1)
            errno = rcode;
        perror("ERROR: Unable to block signals in main thread");
        serial_service_exit(EXIT_FAILURE);
    }

    pthread_t serial_thread; // serial thread, emulator read loop
    pthread_t server_thread; // server listen thread

    // Open serial port
    if (0 > serial_open(0, SERIAL_PORT_BAUDRATE)) {
        printf("ERROR: Unable to open serial port\r\n");
        serial_service_exit(EXIT_SUCCESS);
    }

    // Create serial thread
    rcode = pthread_create(&serial_thread, NULL, serial_port_listen, NULL);
    if (0 != rcode) {
        errno = rcode;
        perror("ERROR: Unable to create serial polling thread");
        serial_service_exit(EXIT_FAILURE);
    }

    // Create TCP/IP server listen thread
    rcode = pthread_create(&server_thread, NULL, serial_server_listen, NULL);
    if (0 != rcode) {
        errno = rcode;
        perror("ERROR: Unable to create serial service server thread");
        serial_service_exit(EXIT_FAILURE);
    }

    // Unblock signals, the main thread can handle them safely
    rcode = serial_mask_signal(SIG_UNBLOCK);
    if (0 != rcode) {
        if (rcode != -1)
            errno = rcode;
        perror("ERROR: Unable to unblock signals in main thread");
        serial_service_exit(EXIT_FAILURE);
    }

    if (0 != pthread_join(serial_thread, NULL)) {
        perror("ERROR: Unable to join serial port polling thread");
    }
    serial_service_exit(EXIT_SUCCESS);
    return 0;
}
