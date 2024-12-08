#include "ftp_socket.h"

int createConnection(char* ip, int port){

    if (ip == NULL){
        fprintf(stderr, "ERROR: NULL parameters\n");
        return -1;
    }

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
        closeConnection(fd);
        return -1;
    }

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


int serverControlResponse(int fd, char* response, int* code) {
    if (response == NULL || code == NULL){
        fprintf(stderr, "ERROR: NULL parameters\n");
        return -1;
    }

    int totalBytes = 0;
    int digitsRead = 0;
    int state = STATE_WAIT_CODE;
    *code = 0;
    memset(response, 0, MAX_CONTROL_SIZE + 1);
    

    // Machine state for reading
    char byte;
    while (state != STATE_STOP) {
        // Read a byte
        int bytesRead = read(fd, &byte, 1);
        if (bytesRead != 1) {
            fprintf(stderr, "ERROR: Read failed\n");
            return -1;
        }
    
        if (totalBytes >= MAX_CONTROL_SIZE + 1){
            fprintf(stderr, "ERROR: Response too large\n");
            return -1;
        }
        response[totalBytes++] = byte;

        // Handle state
        switch (state) {
            case STATE_WAIT_CODE:
                if (isdigit(byte)){
                    digitsRead++;
                    *code += pow(10, 3 - digitsRead)*(byte - '0');
                    if (digitsRead == 3){
                        digitsRead = 0;
                        state = STATE_WAIT_SP;
                    }
                }
                else{
                    digitsRead = 0;
                    *code = 0;
                    state = STATE_WAIT_CODE;
                }
                break;
            case STATE_WAIT_SP:
                if (byte == ' '){
                    state = STATE_WAIT_CR;
                }
                else{
                    digitsRead = 0;
                    *code = 0;
                    state = STATE_WAIT_CODE;
                }
                break;
            case STATE_WAIT_CR:
                if (byte == '\r') {
                    state = STATE_WAIT_LF;
                }
                break;
            case STATE_WAIT_LF:
                if (byte == '\n'){
                    if (totalBytes >= MAX_CONTROL_SIZE + 1){
                        fprintf(stderr, "ERROR: Response too large\n");
                        return -1;
                    }
                    response[totalBytes] = '\0';
                    state = STATE_STOP;
                } 
                break;
            default:
                digitsRead = 0;
                *code = 0;
                state = STATE_WAIT_CODE;
                break;
        }
    }
    return totalBytes; 
}


// Login
int login(char* username, char* password, char* ip, char* host, int port){

    if (username == NULL || password == NULL || ip == NULL || host == NULL){
        fprintf(stderr, "ERROR: NULL parameters\n");
        return -1;
    }

    // Open connection
    control_fd = createConnection(ip,port);
    if (control_fd == -1){
        return -1;
    }
    printf("Client: Connection open on ip:%s, hostname:%s, port:%d\n",ip,host,port);

    // Receive server response
    char response[MAX_CONTROL_SIZE + 1];
    int code;
    int bytesRead = serverControlResponse(control_fd, response, &code);
    if (bytesRead == -1){
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (220)
    if (code != SERVER_READY){
        fprintf(stderr,"ERROR: Server code not expected\n");
        closeConnection(control_fd);
        return -1;
    }

    // Credentials
    // Build (USER) command
    int commandLength = strlen(COMMAND_USER) + strlen(username) + 3;
    char commandUser[commandLength + 1];
    sprintf(commandUser, "%s %s\r\n", COMMAND_USER, username);

    // Send (USER) command
    int bytesSent = write(control_fd, commandUser, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        return -1;
    }
    printf("Client: %s", commandUser);

    // Receive server response
    bytesRead = serverControlResponse(control_fd, response, &code);
    if (bytesRead == -1){
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (331)
    if (code != SERVER_SPECIFY_PASSWORD){
        fprintf(stderr,"ERROR: Server code not expected\n");
        closeConnection(control_fd);
        return -1;
    }

    // Build (PASS) command
    commandLength = strlen(COMMAND_PASS) + strlen(password) + 3;
    char commandPass[commandLength + 1];
    sprintf(commandPass, "%s %s\r\n", COMMAND_PASS, password);

    // Send (PASS) command
    bytesSent = write(control_fd, commandPass, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        return -1;
    }
    printf("Client: %s", commandPass);

    // Receive server response
    bytesRead = serverControlResponse(control_fd, response, &code);
    if (bytesRead == -1){
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (230)
    if (code != SERVER_LOGGED_IN) {
        fprintf(stderr,"ERROR: Server code not expected\n");
        closeConnection(control_fd);
        return -1;
    }
    
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
    
    // Send (QUIT) command
    int commandLength = strlen(COMMAND_QUIT);
    int bytesSent = write(control_fd, COMMAND_QUIT, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        return -1;
    }
    printf("Client: %s", COMMAND_QUIT);

    // Receive server response
    char response[MAX_CONTROL_SIZE + 1];
    int code;
    int bytesRead = serverControlResponse(control_fd, response, &code);
    if (bytesRead == -1){
        closeConnection(control_fd);
        return -1;
    }
    printf("Server: %s\n", response);

    // Check code (221)
    if (code != SERVER_QUIT){
        fprintf(stderr,"ERROR: Server code not expected\n");
        closeConnection(control_fd);
        return -1;
    }

    return closeConnection(control_fd);
}