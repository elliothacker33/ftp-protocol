#include "ftp.h"

int main(int argc, char **argv){
    
    // Parse command line arguments
    if (argc != 2){
        fprintf(stderr,"ERROR: Incorrect command line arguments.\n");
        printf("Example: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    // Parse other arguments
    FTP_Parameters ftpParams;
    
    if (ftpUrlParser(argv[1], &ftpParams) != 0){
        fprintf(stderr,"ERROR: Invalid ftp URL.\n");
        exit(-1);
    }

    // FTP server - Connect (PORT)

    // FTP server - Login ((USR;PASS))
    
    // FTP server - Find file (CWD)

    // FTP server - Change to binary format (TYPE)

    // FTP server - Get file size (SIZE)

    // FTP server - Assert mode (MODE) 

    // FTP server - Download file (RETR)

    // FTP server - Logout (QUIT)


    return 0;
}