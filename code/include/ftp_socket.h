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
#define URL_FIELD_MAX_LENGTH 255
#define URL_MAX_CWD 20
#define LOCAL_PATH_MAX_LENGTH 512
#define DATA_CHUNK_SIZE 100

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
#define SERVER_NEED_ACCT 332

// Code 400
#define SERVER_NOT_AVAILABLE 421
#define SERVER_CANNOT_OPEN_DATA_CONNECTION 425
#define SERVER_DATA_CONNECTION_CLOSED_TRANSFER_ABORTED 426
#define SERVER_REQUESTED_FILE_ACTION_ABORTED 450
#define SERVER_REQUESTED_ACTION_ABORTED 451

// Code 500
#define SERVER_BAD_COMMAND 500
#define SERVER_BAD_PARAMETERS 501
#define SERVER_COMMAND_NOT_IMPLEMENTED 502
#define SERVER_BAD_SEQUENCE_COMMANDS 503
#define SERVER_COMMAND_NOT_IMPLEMENTED_FOR_PARAMETER 504
#define SERVER_NOT_LOGGED_IN 530
#define SERVER_FILE_UNAVAILABLE 550

// Commands
#define COMMAND_USER "USER"
#define COMMAND_PASS "PASS"
#define COMMAND_CWD "CWD"
#define COMMAND_TYPE "TYPE"
#define COMMAND_SIZE "SIZE"
#define COMMAND_PASV "PASV"
#define COMMAND_RETR "RETR"
#define COMMAND_QUIT "QUIT"
#define COMMAND_NLST "NLST"
#define COMMAND_DEFAULT_SIZE 512

// Error codes
#define SUCCESS 0

// Sockets
#define ERROR_OPEN_SOCKET -1
#define ERROR_CLOSE_SOCKET -2
#define ERROR_OPEN_SOCKET_FAILED -3
#define ERROR_CLOSE_SOCKET_FAILED -4
#define ERROR_READ_SOCKET_FAILED -5
#define ERROR_WRITE_SOCKET_FAILED -6

// Server
#define ERROR_CONNECTION_SERVER_FAILED -7
#define ERROR_SERVER_CLOSED_CONNECTION -8
#define ERROR_SERVER_RESPONSE_FAILED -9
#define ERROR_SERVER_CODE -10
#define ERROR_SERVER_CODE_EXPECTED -11
#define ERROR_SERVER_CODE_NOT_EXPECTED -12
#define CHANGE_SERVER_REDIRECT_PASSWORD -13

// File
#define ERROR_WRITE_FILE_FAILED -14
#define ERROR_OPEN_FILE -15
#define ERROR_CLOSE_FILE -16
#define ERROR_FILE_SIZE_MISMATCH -17

// Login
#define ERROR_OPEN_CONTROL_CONNECTION -18
#define ERROR_AUTHENTICATION_FAILED -19

// Download
#define ERROR_CWD -20
#define ERROR_CHANGE_TYPE -21
#define ERROR_GET_FILE_SIZE -22
#define ERROR_ENTER_PASSIVE_MODE -23
#define ERROR_OPEN_DATA_CONNECTION -24
#define ERROR_DOWNLOAD_FILE -25

// List files
#define ERROR_LIST_FILES -26

// Others
#define ERROR_PARSE -27
#define ERROR_EXCEEDED_MAX_ARRAY_SIZE -28

// States
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
 * @brief This function opens a socket at the specified server address
 * @param ip - The server IP address
 * @param port - The server port
 * @param server_addr - The server address
 */
int openSocket(const char* ip, const int port, struct sockaddr_in* server_addr);

/**
 * @brief This function closes a socket
 * @param fd - The file descriptor
 */
int closeSocket(int fd);

/**
 * @brief This function reads control responses from the server
 * @param responseControl - The control response
 * @param code - The control response code
 */
int serverResponseControl(char *responseControl, int *code);

/**
 * @brief This function reads data from the server
 */
int serverResponseData();

/**
 * @brief Process response code, (RFC959 codes)
 * @param code - Server code
 * @param possibleCodes - List of possible codes for a command
 * @param command - Command sent to server
 * @param response - Server response
 */
int processServerCode(const int code, const int* possibleCodes, const char* command, const char* response);

/**
 * @brief This function opens a control connection to the server
 * @param ip - The server IP address
 * @param host - The server host name
 * @param port - The server port
 */
int openControlConnection(const char* ip, const char* host, const int port);

/**
 * @brief This function opens a data connection to the server
 * @param ip - The server IP address
 * @param port - The server port
 */
int openDataConnection(const char* ip, const int port);

/**
 * @brief Authenticate user with username and password.
 * @param username - User name
 * @param password - Password
 */
int authenticateCredentials(const char* username, const char* password);

/**
 * @brief Logic of connection and authentication
 * @param username - User name
 * @param password - Password
 * @param ip - The server IP address
 * @param host - The server host name
 * @param port - The server port
 */
int login(const char* username, const char* password, const char* ip, const char* host, const int port);

/**
 * @brief Go to specified directory
 * @param directories - List of directories
 */
int cwd(char directories[URL_MAX_CWD + 1][URL_FIELD_MAX_LENGTH + 1]);

/**
 * @brief Set type for file transfer
 * @param typecode - File transfer type
 */
int changeType(const char typecode);

/**
 * @brief Get file size of local file
 * @param fptr - File pointer
 */
long getLocalFileSize(FILE* fptr);

/**
 * @brief Get size of the file requested in URL
 * @param filename - File name
 * @param fileSize - File size (Result)
 */
int getFileSize(const char* filename, int* fileSize);

/**
 * @brief Open passive mode for file transfer
 * @param ip - Ip address for file transfer (Result)
 * @param port - Port number for file transfer (Result)
 */
int enterPassiveMode(char* ip, int* port);

/**
 * @brief Retrieve file or List files
 * @param filename - File name
 * @param localPath - File path on local filesystem (save path)
 * @param localfileSize - File size that arrived
 * @param typecode - File transfer type
 */
int downloadFile(const char* filename, const char* localPath, int* localFileSize, const char typecode);

/**
 * @brief Logic of download file
 * @param directories - List of directories
 * @param filename - File name
 * @param typecode - File transfer type
 */
int download(char directories[URL_MAX_CWD + 1][URL_FIELD_MAX_LENGTH + 1], const char* filename, const char typecode);

/**
 * @brief Disconnect from server
 */
int logout();


#endif // _FTP_SOCKET_H_