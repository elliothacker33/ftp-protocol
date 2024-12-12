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
#define MAX_CONTROL_SIZE 10000
#define SERVER_WAIT_N 120
#define SERVER_DATA_CONNECTION_OPEN 125
#define SERVER_OPENING_DATA_CONNECTION 150

// Code 200
#define SERVER_COMMAND_OK 200
#define SERVER_COMMAND_NOT_IMPLEMENTED_SUPERFLUOUS 202
#define SERVER_FILE_STATUS 213
#define SERVER_READY 220
#define SERVER_QUIT 221
#define SERVER_DATA_CONNECTION_CLOSE 226
#define SERVER_ENTERING_PASSIVE_MODE 227
#define SERVER_LOGGED_IN 230
#define SERVER_FILE_ACTION_OK 250

// Code 300
#define SERVER_SPECIFY_PASSWORD 331
#define PASSWORD 1
#define SERVER_NEED_ACCT 332

// code 400
#define SERVER_NOT_AVAILABLE 421
#define SERVER_CANNOT_OPEN_DATA_CONNECTION 425
#define SERVER_DATA_CONNECTION_CLOSED_TRANSFER_ABORTED 426
#define SERVER_REQUESTED_FILE_ACTION_ABORTED 450
#define SERVER_REQUESTED_ACTION_ABORTED 451

// code 500
#define SERVER_BAD_COMMAND 500
#define SERVER_BAD_PARAMETERS 501
#define SERVER_COMMAND_NOT_IMPLEMENTED 502
#define SERVER_BAD_SEQUENCE_COMMANDS 503
#define SERVER_COMMAND_NOT_IMPLEMENTED_FOR_PARAMETER 504
#define SERVER_NOT_LOGGED_IN 530
#define SERVER_FILE_UNAVAILABLE 550

// Timeout sockets
#define MAX_TIMEOUT_TRIES 5
#define SECONDS 2
#define MILLISECONDS 0

// Commands
#define COMMAND_USER "USER"
#define COMMAND_PASS "PASS"
#define COMMAND_CWD "CWD"
#define COMMAND_TYPE "TYPE"
#define COMMAND_SIZE "SIZE"
#define COMMAND_PASV "PASV"
#define COMMAND_RETR "RETR"
#define COMMAND_QUIT "QUIT\r\n"

// Error codes
#define SUCCESS 0
#define ERROR_SOCKET_CREATION_FAILED -1
#define ERROR_CONNECTION_SERVER_FAILED -2
#define ERROR_NULL_PARAMETERS -3
#define ERROR_CLOSING_SOCKET -4
#define ERROR_EXCEEDED_MAX_SIZE -6
#define ERROR_SERVER_CLOSED_CONNECTION -7
#define ERROR_READ_SOCKET_FAILED -8
#define ERROR_WRITE_SOCKET_FAILED -9
#define ERROR_WRITE_FILE_FAILED -10
#define ERROR_MAX_TIMEOUT -11
#define ERROR_SERVER_RESPONSE_FAILED -12
#define ERROR_SERVER_CODE -13
#define ERROR_OPEN_CONTROL_CONNECTION -14
#define ERROR_AUTHENTICATION_FAILED -15
#define ERROR_CWD_FAILED -16
#define ERROR_PARSE -17
#define ERROR_GET_SIZE_FAILED -18
#define ERROR_INVALID_TYPECODE -19
#define ERROR_CHANGE_TYPE_FAILED -20
#define ERROR_ENTER_PASSIVE_MODE_FAILED -21
#define ERROR_OPEN_FILE -22
#define ERROR_DOWNLOAD_FILE_FAILED -23

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
 * @param port - Port number
 */
int createConnection(const char* ip, const int port);

/**
 * @brief Close a connection on file descriptor fd
 * @param fd - File descriptor
 */
int closeConnection(int fd);

/**
 * @brief Open control connection
 * @param ip - IP address
 * @param host - Host name
 * @param port - Port number 
 */
int openControlConnection(const char* ip, const char* host, const int port);

/**
 * @brief Authenticate user with username and password.
 * @param username - User name
 * @param password - Password
 */
int authenticateCredentials(const char* username, const char* password);

/**
 * @brief Read server control responses
 * @param response - Server response
 * @param code - Server response code
 */
int serverResponse(char* response, int* code);

/**
 * @brief Process response code, (RFC959 codes)
 * @param code - Server code
 * @param possibleCodes - List of possible codes for a command
 * @param command - Command sent to server
 * @param response - Server response
 */
int processServerCode(const int code, const int* possibleCodes, const char* command, const char* response);

/**
 * @brief Login user account
 * @param username - User name
 * @param password - Password
 * @param ip - IP address
 * @param port - Port number
 * @param host - Host name
 */

/**
 * @brief Change working directory
 * @param directiories - All directories to do CWD
 */
int cwd(char directories[21][256]);

/**
 * @brief Download a resource from the server
 * @param directories - List of directories
 * @param filename - File name
 * @param typecode - (TYPE)
 */
int download(char directories[21][256], const char* filename, const char typecode);

int login(const char *username, const char *password, const char* ip, const char* host, const int port);

/**
 * @brief Disconnect from server
 */
int logout();

#endif // _FTP_SOCKET_H_