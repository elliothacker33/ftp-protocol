#include "ftp_socket.h"

int control_fd;
int data_fd;

int createConnection(char* ip, int port){
    // Socket settings
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = IPV4;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    
    // Create socket
    int fd = socket(IPV4, CONNECTION_TYPE, 0) ;
    if (fd < 0){
        fprintf(stderr, "ERROR: Socket creation failed\n");
        return -1;
    }

    // Connect to server on IP + port
    if (connect(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "ERROR: Connection failed\n");
        return -2;
    }
    printf("Client: Connection open on %s\n",ip);

    return fd;
}

int closeConnection(int fd){
    // Close socket
    if (close(fd) == -1){
        fprintf(stderr, "ERROR: Socket closing failed\n");
        return -1;
    };
    return 0;
}


int serverResponse(int fd, char** response) {
    // Allocate initial memory
    *response = malloc(sizeof(char));
    if (*response == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        return -1;
    }

    int totalBytes = 1;
    char byte;
    int digitsRead = 0;
    int state = STATE_WAIT_CODE;

    // Machine state for reading
    while (state != STATE_STOP) {
        int bytesRead = read(fd, &byte, 1);
        if (bytesRead != 1) {
            fprintf(stderr, "ERROR: Read failed\n");
            free(*response);
            return -1;
        }

        char* temp = realloc(*response, totalBytes + 1); 
        if (temp == NULL) {
            fprintf(stderr, "ERROR: Memory reallocation failed\n");
            free(*response);
            *response = NULL;
            return -1;
        }
        *response = temp;
        (*response)[totalBytes - 1] = byte;
        totalBytes++;
    
        switch (state) {
            case STATE_WAIT_CODE:
                if (isdigit(byte)){
                    digitsRead++;
                    if (digitsRead == 3){
                        digitsRead = 0;
                        state = STATE_WAIT_SP;
                    }
                    break;
                }
                else{
                    state = STATE_WAIT_CODE;
                    break;
                }
            case STATE_WAIT_SP:
                if (byte == SP){
                    state = STATE_WAIT_CR;
                }
                else{
                    state = STATE_WAIT_CODE;
                }
                break;

            case STATE_WAIT_CR:
                if (byte == CR) {
                    state = STATE_WAIT_LF;
                }
                break;
            case STATE_WAIT_LF:
                if (byte == LF){
                    (*response)[totalBytes] = '\0';
                    state = STATE_STOP;
                } 
                break;
            default:
                state = STATE_WAIT_CODE;
                break;
        }
    }
    return totalBytes - 1; 
}


// Login
int login(char *username, char *password, char* ip, int port){
    // Open connection
    control_fd = createConnection(ip,port);
    if (control_fd == -1){
        return -1;
    }
    else if (control_fd == -2) {
        closeConnection(control_fd);
        return -1;
    }
    
    char* response = NULL;
    int bytesRead = serverResponse(control_fd, &response);
    if (bytesRead == -1 || response == NULL){
        response ? NULL : free(response);
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (220)
    if (strncmp(response, SERVER_READY, CODE_SIZE) != 0){
        fprintf(stderr,"ERROR: Server code not expected\n");
        free(response);
        closeConnection(control_fd);
        return -1;
    }
    free(response);

    // Credentials
    // Username
    char* commandType = "USER";
    int commandLength = strlen(commandType) + strlen(username) + 3;
    char commandUser[commandLength + 1];
    sprintf(commandUser, "%s %s\r\n", commandType, username);

    int bytesSent = write(control_fd, commandUser, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        return -1;
    }
    printf("Client: %s", commandUser);

    bytesRead = serverResponse(control_fd, &response);
    if (bytesRead == -1 || response == NULL){
        response ? NULL : free(response);
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (331)
    if (strncmp(response, SERVER_SPECIFY_PASSWORD, CODE_SIZE) != 0){
        fprintf(stderr,"ERROR: Server code not expected\n");
        free(response);
        closeConnection(control_fd);
        return -1;
    }
    free(response);

    // Password
    commandType = "PASS";
    commandLength = strlen(commandType) + strlen(password) + 3;
    char commandPass[commandLength + 1];
    sprintf(commandPass, "%s %s\r\n", commandType, password);

    bytesSent = write(control_fd, commandPass, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        return -1;
    }
    printf("Client: %s", commandPass);

    bytesRead = serverResponse(control_fd, &response);
    if (bytesRead == -1 || response == NULL){
        response ? NULL : free(response);
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (331)
    if (strncmp(response, SERVER_LOGGED_IN, CODE_SIZE) != 0){
        fprintf(stderr,"ERROR: Server code not expected\n");
        free(response);
        closeConnection(control_fd);
        return -1;
    }
    free(response);
    
    return 0;
}



// Download
// FTP server - Find file (CWD)
// FTP server - Change to binary format (TYPE)
// FTP server - Get file size (SIZE)
// FTP server - Assert mode (MODE) 
// FTP server - Download file (RETR)
//int download(){}

// Logout
int logout(){
    // Username
    char* commandQuit = "QUIT\r\n";
    int commandLength = strlen(commandQuit);

    int bytesSent = write(control_fd, commandQuit, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        return -1;
    }
    printf("Client: %s", commandQuit);

    char* response = NULL;
    int bytesRead = serverResponse(control_fd, &response);
    if (bytesRead == -1 || response == NULL){
        response ? NULL : free(response);
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (221)
    if (strncmp(response, SERVER_QUIT, CODE_SIZE) != 0){
        fprintf(stderr,"ERROR: Server code not expected\n");
        free(response);
        closeConnection(control_fd);
        return -1;
    }
    free(response);

    return closeConnection(control_fd);
}