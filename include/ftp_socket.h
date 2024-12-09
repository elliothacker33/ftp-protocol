#ifndef _FTP_SOCKET_H_
#define _FTP_SOCKET_H_

// Includes
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/select.h>
#include <math.h>

// Connection settings
#define IPV4 AF_INET
#define CONNECTION_TYPE SOCK_STREAM

// Server code settings
#define CODE_SIZE 3
#define MAX_CONTROL_SIZE 1024
#define SERVER_WAIT_N 120
#define SERVER_COMMAND_NOT_IMPLEMENTED 202
#define SERVER_READY 220
#define SERVER_QUIT 221
#define SERVER_LOGGED_IN 230
#define SERVER_SPECIFY_PASSWORD 331
#define SERVER_NEED_ACCT 332
#define SERVER_NOT_AVAILABLE 421
#define SERVER_BAD_COMMAND 500
#define SERVER_BAD_PARAMETERS 501
#define SERVER_NOT_LOGGED_IN 530
// Timeout sockets
#define MAX_TIMEOUT_TRIES 5
#define SECONDS 2
#define MILLISECONDS 0
// Commands
#define COMMAND_USER "USER"
#define COMMAND_PASS "PASS"
#define COMMAND_QUIT "QUIT\r\n"

// States
/**
 * @enum State - States for reading control responses from the server
 */
typedef enum {
    STATE_WAIT_CODE,
    STATE_WAIT_SP,
    STATE_WAIT_CR,
    STATE_WAIT_LF,
    STATE_STOP
} ControlState;

typedef enum {
    STATE_WAIT_DATA,
    STATE_FULL_FILE_READ,
} DataState;

/**
 * @brief Open a connection to the server on ip,port.
 * @param ip - IP address
 * @param port - Port
 */
int createConnection(char* ip, int port);

/**
 * @brief Close open sockets (data and/or control)
 * @return - 0 on success, -1 on failure
 */
int closeConnections();

/**
 * @brief Read server control responses
 * @param response - Server response
 * @param code - Server response code
 * @return - 0 on success, -1 on failure
 */
int serverResponse(char* response, int* code);

/**
 * @brief Process response code, (RFC959 codes)
 * @param code - Server code
 * @param possibleCodes - List of possible codes for a command
 * @param command - Command sent to server
 * @param response - Server response
 * @return - 0 on success, 1 (password feature), -1 on failure
 */
int processServerCode(int code, int* possibleCodes, char* command, char* response);

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

#endif // _FTP_SOCKET_H_