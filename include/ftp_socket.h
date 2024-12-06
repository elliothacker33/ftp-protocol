#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// Connection settings
#define IPV4 AF_INET
#define CONNECTION_TYPE SOCK_STREAM
// Server code
#define CODE_SIZE 3
#define SERVER_READY "220"
#define SERVER_SPECIFY_PASSWORD "331"
#define SERVER_LOGGED_IN "230"
#define SERVER_QUIT "221"
// String
#define SP ' '
#define CR '\r'
#define LF '\n'

// Booleans
#define TRUE 1
#define FALSE 0
// States
typedef enum {
    STATE_WAIT_CODE,
    STATE_WAIT_SP,
    STATE_WAIT_CR,
    STATE_WAIT_LF,
    STATE_STOP
} State;

/**
 * @brief Open a connection to the server on ip,port.
 * @param ip - IP address
 * @param port - Port
 */
int createConnection(char* ip, int port);

/**
 * @brief Close the socket
 * @param fd - File descriptor
 */
int closeConnection(int fd);

/**
 * @brief Read server responses
 * @param fd - File descriptor
 * @param response - Server response
 */
int serverResponse(int fd, char** response);

/**
 * @brief Login user account
 * @param username - User name
 * @param password - Password
 * @param ip - IP address
 * @param port - Port
 */
int login(char *username, char *password, char* ip, int port);

/**
 * @brief Disconnect from server
 */
int logout();