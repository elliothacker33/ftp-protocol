#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define FTP_PORT 21
#define MAX_LENGTH_URL 2048
#define PASSIVE_MODE 0
#define ACTIVE_MODE 1


typedef struct {
    char* username;
    char* password;
    char* host;
    char* urlPath;
} FTP_Parameters;

// Implementation parse file transfer protocol url
int parse_ftp_url(const char* url, FTP_Parameters* parameters){

    int totalUrlLength = 0;
    // URL start 
    if (strncmp(url,"ftp://",6) != 0){
        printf("Invalid url start"); 
        return -1;
    }

    url += 6;
    totalUrlLength += 6;

    parameters->username = NULL;
    parameters->password = NULL;
    parameters->host = NULL;
    parameters->urlPath = NULL;

    // Username and password logic 
    char* posArr = strchr(url,'@');
    if (posArr != NULL){
        char* posColon = strchr(url, ':');
        if (posColon != NULL && posColon < posArr){
            
            // Username
            int usernamesLength = posColon - url;
            totalUrlLength += usernamesLength + 1;

            if (totalUrlLength > MAX_LENGTH_URL){
                printf("URL is too long\n");
                return -1;
            }

            parameters->username = malloc(usernamesLength + 1);
            if (parameters->username == NULL){
                printf("ERROR: Failed to allocate memory\n");
                return -1;
            }

            strncpy(parameters->username, url, usernamesLength);
            parameters->username[usernamesLength] = '\0';

            // Password
            int passwordLength = posArr - posColon - 1;
            totalUrlLength += passwordLength + 1;

            if (totalUrlLength > MAX_LENGTH_URL){
                printf("URL is too long\n");
                return -1;
            }

            parameters->password = malloc(passwordLength + 1);
            if (parameters->password == NULL){
                printf("ERROR: Failed to allocate memory\n");
                return -1;
            }

            strncpy(parameters->password,posColon + 1,passwordLength);
            parameters->password[passwordLength] = '\0';
            
        }
        else{
            // Only has username, but no password
            int usernameLength = posArr - url;
            totalUrlLength += usernameLength + 1;

            if (totalUrlLength > MAX_LENGTH_URL){
                printf("URL is too long\n");
                return -1;
            }

            parameters->username = malloc(usernameLength + 1);
            if (parameters->username == NULL){
                printf("ERROR: Failed to allocate memory\n");
                return -1;
            }
            strncpy(parameters->username, url, usernameLength);
            parameters->username[usernameLength] = '\0';

            parameters->password = malloc(1);
            if (parameters->password == NULL){
                printf("ERROR: Failed to allocate memory\n");
                return -1;
            }
            parameters->password[0] = '\0';

        }
        url = posArr + 1;
    }
    else{
     
        parameters->username = malloc(1);
        if (parameters->username == NULL){
            printf("ERROR: Failed to allocate memory\n");
            return -1;
        }
        parameters->username[0] = '\0';

        parameters->password = malloc(1);
        if (parameters->password == NULL){
            printf("ERROR: Failed to allocate memory\n");
            return -1;
        }
        parameters->password[0] = '\0';
    }

    // Host logic

    char* posSlash = strchr(url, '/');
    if (posSlash != NULL){

        int hostLength = posSlash - url;
        totalUrlLength += hostLength + 1;

        if (totalUrlLength > MAX_LENGTH_URL){
            printf("URL is too long\n");
            return -1;
        }

        parameters->host = malloc(hostLength + 1);
        if (parameters->host == NULL){
            printf("ERROR: Failed to allocate memory\n");
            return -1;
        }
        strncpy(parameters->host, url, hostLength);
        parameters->host[hostLength] = '\0';

        url = posSlash + 1;

    }
    else{
        printf("ERROR: Invalid ftp URL. Host is required\n");
        return -1;
    }

    // URL path logic
    int urlPathLength = strlen(url);

    if (urlPathLength == 0){
        // No URL path, so set it to an empty string
        parameters->urlPath = malloc(1);
        if (parameters->urlPath == NULL){
            printf("ERROR: Failed to allocate memory\n");
            return -1;
        }
        parameters->urlPath[0] = '\0';
        return 0;
    }

    totalUrlLength += urlPathLength;

    if(totalUrlLength > MAX_LENGTH_URL){
        printf("URL is too long\n");
        return -1;
    }
    parameters->urlPath = malloc(urlPathLength + 1);
    if (parameters->urlPath == NULL){
        printf("ERROR: Failed to allocate memory\n");
        return -1;
    }
    strcpy(parameters->urlPath, url);

    printf("URL: %s\n", parameters->urlPath);
    printf("Username: %s\n", parameters->username);
    printf("Password: %s\n", parameters->password);
    printf("Host: %s\n", parameters->host);
    printf("chars: %d\n", totalUrlLength);
    return 0;

}

int main(int argc, char **argv){
    
    // Parse command line arguments

    if (argc != 2){
        printf("ERROR: Incorrect command line arguments.\n");
        printf("Example: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    if (strcmp(argv[0], "./download") != 0){
        printf("ERROR: Invalid command. Use 'download' as the first argument.\n");
        exit(-1);
    }

    // Parse other arguments
    FTP_Parameters ftpParams;
    memset(&ftpParams, 0, sizeof(FTP_Parameters));

    if (parse_ftp_url(argv[1], &ftpParams) != 0){
        printf("ERROR: Invalid ftp URL.\n");
        exit(-1);
    }

    // FTP server - Login ((USR;PASS))
    
    // FTP server - Find file (CWD)

    // FTP server - Change to binary format (TYPE)

    // FTP server - Get file size (SIZE)

    // FTP server - Assert mode (MODE) 

    // FTP server - Download file (RETR)

    // FTP server - Logout (QUIT)

    // Free allocated memory
    if (ftpParams.username != NULL){
        free(ftpParams.username);
    }

    if (ftpParams.password != NULL){
        free(ftpParams.password);
    }

    if (ftpParams.host != NULL){
        free(ftpParams.host);
    }

    if (ftpParams.urlPath != NULL){
        free(ftpParams.urlPath);
    }

    return 0;
}