
#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "dns.h"
#define URL_FIELD_MAX_LENGTH 255
#define URL_MAX_CWD 20



typedef struct {
    char username[URL_FIELD_MAX_LENGTH + 1];
    char password[URL_FIELD_MAX_LENGTH + 1];
    char hostname[URL_FIELD_MAX_LENGTH + 1];
    char directories[URL_MAX_CWD + 1][URL_FIELD_MAX_LENGTH + 1];
    char filename[URL_FIELD_MAX_LENGTH + 1];
    char ip[URL_FIELD_MAX_LENGTH + 1]; 
    char typecode;
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