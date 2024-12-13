#ifndef _PARSER_H_
#define _PARSER_H_

// Includes
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Modules
#include "dns.h"

// Url settings
#define URL_FIELD_MAX_LENGTH 255
#define URL_MAX_CWD 20
#define FTP_DEFAULT_PORT 21
#define FTP_DEFAULT_TYPE_CODE "i"
#define FTP_PREFIX_SIZE 6
#define FTP_MAX_PORT 65535
#define URL_MAX_PORT_LENGTH 15

// Anonymous account
#define USER_ANONYMOUS "anonymous"
#define PASS_ANONYMOUS "upXXXXXXXXX@edu.fe.up.pt" // Email address or "anonymous"


// Error codes
#define SUCCESS 0
#define ERROR_MEMORY_ALLOCATION -1;
#define ERROR_INVALID_HOST_NANE -2;
#define ERROR_INVALID_URL_FORMAT -3;
#define ERROR_FIELD_SIZE_EXCEEDED -4;
#define ERROR_DECODE -5;
#define ERROR_HOST_NOT_FOUND -6;


/**
 * @struct Parameters for a ftp connection
 */
typedef struct {
    char username[URL_FIELD_MAX_LENGTH + 1];
    char password[URL_FIELD_MAX_LENGTH + 1];
    char hostname[URL_FIELD_MAX_LENGTH + 1];
    char directories[URL_MAX_CWD + 1][URL_FIELD_MAX_LENGTH + 1];
    char filename[URL_FIELD_MAX_LENGTH + 1];
    char ip[URL_FIELD_MAX_LENGTH + 1]; 
    char typecode; // I - Image (binary) A - Ascii
    int port;
} FTP_Parameters;

/**
 * @brief This function decodes percent-encoded characters in the given url parameter.
 * @param parameter - pointer to the string to decode
 * @return -1 in case of error, 0 otherwise
 */
int decodePercent(char* parameter);

/**
 * @brief This function is used to check if ip and hostname are valid
 * @param hostname - hostname address
 * @param ip - ip address
 * @return -1 in case of error, 0 otherwise.
 */
int ipAndHostChecker(char* hostname, char* ip);

/**
 * @brief This function is used to parse the ftp url.
 * @param url - pointer to the url.
 * @param parameters - pointer to the FTP_Parameters struct where the parsed data will be stored.
 * @return -1 in case of error, 0 otherwise
 */
int ftpUrlParser(const char* url, FTP_Parameters* parameters);

#endif // _PARSER_H_