#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Connection settings
#define IPV4 AF_INET
#define CONNECTION_TYPE SOCK_STREAM
// Server code
#define CODE_SIZE 3
#define MAX_CONTROL_SIZE 1024
#define SERVER_READY 220
#define SERVER_SPECIFY_PASSWORD 331
#define SERVER_LOGGED_IN 230
#define SERVER_QUIT 221
// Commands
#define COMMAND_USER "USER"
#define COMMAND_PASS "PASS"
#define COMMAND_QUIT "QUIT\r\n"

// States
typedef enum {
    STATE_WAIT_CODE,
    STATE_WAIT_SP,
    STATE_WAIT_CR,
    STATE_WAIT_LF,
    STATE_STOP
} State;

// File descriptor for control and data 
int control_fd;
int data_fd;

/**
 * @brief Open a connection to the server on ip,port.
 * @param ip - IP address
 * @param port - Port
 */
int createConnection(char* ip, int port);

/**
 * @brief Close the socket
 * @param fd - File descriptor
 * @return - 0 on success, -1 on failure
 */
int closeConnection(int fd);

/**
 * @brief Read server responses
 * @param fd - File descriptor
 * @param response - Server response
 * @param code - Server response code
 * @return - 0 on success, -1 on failure
 */
int serverResponse(int fd, char* response, int* code);

/**
 * @brief Login user account
 * @param username - User name
 * @param password - Password
 * @param ip - IP address
 * @param port - Port
 * @param host - Hostname
 * @return - 0 on success, -1 on failure
 */
int login(char *username, char *password, char* ip, char* host, int port);

/**
 * @brief Disconnect from server
 * @return 0 on success, -1 on failure
 */
int logout();