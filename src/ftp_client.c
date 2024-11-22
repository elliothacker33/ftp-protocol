#include "ftp_client.h"

int main(int argc, char **argv){
    
    // Parse command line arguments
    if (argc != 2){
        fprintf(stderr,"ERROR: Incorrect command line arguments.\n");
        printf("Example: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }
    
    FTP_Parameters ftpParams;
    memset(&ftpParams, 0, sizeof(FTP_Parameters));
    if (ftpUrlParser(argv[1], &ftpParams) != 0){
        fprintf(stderr,"ERROR: Invalid ftp URL.\n");
        exit(-1);
    }

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
    printf("Typecode: %c\n", ftpParams.typecode);


    // Login

    // Download

    // Logout

    return 0;
}