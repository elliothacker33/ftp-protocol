#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define URL_FIELD_MAX_LENGTH 255
#define URL_MAX_PATH_LENGTH  1024

typedef struct {
    char username[URL_FIELD_MAX_LENGTH + 1];
    char password[URL_FIELD_MAX_LENGTH + 1];
    char hostname[URL_FIELD_MAX_LENGTH + 1];
    char path[URL_MAX_PATH_LENGTH + 1];
    char ip[URL_FIELD_MAX_LENGTH + 1]; // IPV4
    int port;
} FTP_Parameters;


/**
 * @brief This function is used to parse the ftp url.
 * @param url - pointer to the url.
 * @param parameters - pointer to the FTP_Parameters struct where the parsed data will be stored.
 */
int parse_ftp_url(const char* url, FTP_Parameters* parameters);

/**
 * @brief This function handles the logic of a download request to a ftp server.
 */
int main(int argc, char **argv);