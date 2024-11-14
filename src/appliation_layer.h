#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char* username;
    char* password;
    char* host;
    char* urlPath;
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