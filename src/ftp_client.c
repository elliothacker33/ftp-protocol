#include "ftp_client.h"

int main(int argc, char **argv){
    
    // Parse command line arguments
    if (argc != 2){
        fprintf(stderr,"ERROR: Incorrect command line arguments.\n");
        printf("Example: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }
    
    FTP_Parameters ftpParams;
    memset(&ftpParams, 0, sizeof(FTP_Parameters));
    if (ftpUrlParser(argv[1], &ftpParams) != 0){
        fprintf(stderr,"ERROR: Invalid ftp URL.\n");
        return -1;
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
    if (login(ftpParams.username, ftpParams.password, ftpParams.ip, ftpParams.hostname, ftpParams.port) == -1){
        fprintf(stderr,"ERROR: Login failed.\n");
        return -1;
    }

    // Download

    // Logout
    if (logout() == -1){
        fprintf(stderr,"ERROR: Logout failed.\n");
        return -1;
    }
    return 0;
}