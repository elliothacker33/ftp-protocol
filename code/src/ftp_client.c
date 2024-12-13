#include "ftp_client.h"

int main(int argc, char **argv){
    
    // Parse command line arguments
    if (argc != 2){
        fprintf(stderr,"ERROR: Incorrect command line arguments.\n");
        printf("Example: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        return ERROR_COMMAND_LINE_ARGS;
    }
    
    FTP_Parameters ftpParams;
    memset(&ftpParams, 0, sizeof(FTP_Parameters));
    if (ftpUrlParser(argv[1], &ftpParams) != SUCCESS){
        fprintf(stderr,"ERROR: Invalid ftp URL.\n");
        return ERROR_INVALID_URL;
    }
    printf("URL PARSER RESULTS\n\n");
    // Pararameters
    printf("Username: %s\n", ftpParams.username);
    printf("Password: %s\n", ftpParams.password);
    printf("Host: %s\n", ftpParams.hostname);
    printf("Ip: %s, IPV4\n", ftpParams.ip);
    printf("Port: %d\n", ftpParams.port);
    int i = 0;
    while (ftpParams.directories[i][0] != '\0'){
        printf("Directory %d: %s\n",i+1,ftpParams.directories[i]);
        i++;
    }
    printf("Filename: %s\n", ftpParams.filename);
    printf("Typecode: %c\n\n", ftpParams.typecode);

    printf("CONNECTION CLIENT-SERVER\n\n");

    // Login
    if (login(ftpParams.username, ftpParams.password, ftpParams.ip, ftpParams.hostname, ftpParams.port) != SUCCESS){
        fprintf(stderr,"ERROR: Login failed.\n");
        return ERROR_LOGIN_FAILED;
    }
    
    // Download
    if (download(ftpParams.directories, ftpParams.filename, ftpParams.typecode) != SUCCESS){
        fprintf(stderr,"ERROR: Download failed.\n");
        return ERROR_DOWNLOAD_FAILED;
    }
    
    // Logout
    if (logout() != SUCCESS){
        fprintf(stderr,"ERROR: Logout failed.\n");
        return ERROR_LOGOUT_FAILED;
    }

    return SUCCESS;
}