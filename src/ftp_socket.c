#include "ftp_socket.h"

// File descriptor for control and data 
int controlFd = -1;
int dataFd = -1;

int createConnection(char* ip, int port){

    if (!ip){
        fprintf(stderr, "ERROR: NULL parameters\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = IPV4;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);    
    
    int fd = socket(IPV4, CONNECTION_TYPE, 0);
    if (fd < 0){
        fprintf(stderr, "ERROR: Socket creation failed\n");
        return -1;
    }

    if (connect(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "ERROR: Connection failed\n");
        return -1;
    }

    return fd;
}

int closeConnections(){
    // Close control socket
    if (controlFd != -1 && close(controlFd) == -1){
        fprintf(stderr, "ERROR: Closing control socket\n");
        return -1;
    }
    // Close data socket
    if (dataFd != -1 && close(dataFd) == -1){
        fprintf(stderr, "ERROR: Closing data socket\n");
        return -1;
    }

    controlFd = -1;
    dataFd = -1;
    return 0;
}

int closeDataConnection(){
    // Close data socket
    if (dataFd != -1 && close(dataFd) == -1){
        fprintf(stderr, "ERROR: Closing data socket\n");
        return -1;
    }

    dataFd = -1;
    return 0;
}


int serverResponse(char *responseControl, int *code) {

    if (!responseControl || !code || (controlFd == -1 && dataFd == -1)) {
        fprintf(stderr, "ERROR: Invalid input parameters\n");
        return -1;
    }

    int totalBytes = 0, digitsRead = 0, maxFd;
    ControlState stateC = STATE_WAIT_CODE;
    DataState stateD = STATE_WAIT_DATA;

    *code = 0;
    char byte;
    memset(responseControl, 0, MAX_CONTROL_SIZE + 1);

    fd_set readFdSet;
    struct timeval timeout;
    timeout.tv_sec = SECONDS; 
    timeout.tv_usec = MILLISECONDS;  
    int tries = 0;

    while ((stateC != STATE_STOP && controlFd != -1) || (stateD != STATE_FULL_FILE_READ && dataFd != -1)) {
        
        FD_ZERO(&readFdSet);
        if (controlFd != -1) FD_SET(controlFd, &readFdSet);
        if (dataFd != -1) FD_SET(dataFd, &readFdSet);
        maxFd = (controlFd > dataFd) ? controlFd : dataFd;

        int ready = select(maxFd + 1, &readFdSet, NULL, NULL, &timeout);
    
        if (ready != -1) {
            tries = 0; // Reset tries

            if (controlFd != -1 && FD_ISSET(controlFd, &readFdSet) && stateC != STATE_STOP) {
                int bytesRead = read(controlFd, &byte, 1);
                if (bytesRead > 0) {
                    if (totalBytes >= MAX_CONTROL_SIZE) {
                        fprintf(stderr, "ERROR: Response too large\n");
                        return -1;
                    }
                    responseControl[totalBytes++] = byte;

                    switch (stateC) {
                        case STATE_WAIT_CODE:
                            if (isdigit(byte)) {
                                digitsRead++;
                                *code = *code * 10 + (byte - '0');
                                if (digitsRead == 3) stateC = STATE_WAIT_SP;
                            } else {
                                digitsRead = 0;
                                *code = 0;
                            }
                            break;

                        case STATE_WAIT_SP:
                            stateC = (byte == ' ') ? STATE_WAIT_CR : STATE_WAIT_CODE;
                            break;

                        case STATE_WAIT_CR:
                            stateC = (byte == '\r') ? STATE_WAIT_LF : STATE_WAIT_CR;
                            break;

                        case STATE_WAIT_LF:
                            if (byte == '\n') {
                                responseControl[totalBytes] = '\0';
                                stateC = STATE_STOP;
                            }
                            break;

                        default:
                            digitsRead = 0;
                            *code = 0;
                            stateC = STATE_WAIT_CODE;
                            break;
                    }
                } else if (bytesRead == 0) {
                    fprintf(stderr, "ERROR: Server closed connection\n");
                    return -1;
                } else {
                    fprintf(stderr,"ERROR: Read failed");
                    return -1;
                }
            }

            if (dataFd != -1 && FD_ISSET(dataFd, &readFdSet) && stateD != STATE_FULL_FILE_READ) {
                // Data reading (Read until 0 bytes received (server closed connection))
                // If 0 bytes received STATE_DATA_CLOSED_CONNECTION
            }
        }
        else{
            tries++;
            if (tries >= MAX_TIMEOUT_TRIES) {
                fprintf(stderr, "ERROR: Server response timeout\n");
                return -1;
            }
        }
    }

    return 0;
}

int processServerCode(int code, int* possibleCodes, char* command, char* response){
    for(int i = 0; i < sizeof(possibleCodes); i++){
        if (code == possibleCodes[i]){
            switch(code){
                case SERVER_WAIT_N: {
                    int minutes = 0;
                    if (sscanf(response, "120 Service ready in %d minutes", &minutes) == 1 && minutes > 0) {
                        printf("Client: Server requests to wait %d minutes. Sleeping...\n", minutes);
                        sleep(minutes * 60);
                    } else {
                        fprintf(stderr, "ERROR: Failed to parse wait time from server response\n");
                        closeConnections();
                        return -1;
                    }
                    break;
                }
                case SERVER_COMMAND_NOT_IMPLEMENTED:
                    printf("Client: Command -> %s, not implemented on this server\n", command);
                    printf("Help: Type 'FEAT' and 'HELP' to see all available commands"); 
                    closeConnections(); 
                    return -1;
                case SERVER_READY:
                    printf("Client: Server is ready\n");
                    break;
                case SERVER_QUIT:
                    printf("Client: Server quit\n");
                    return closeConnections();
                case SERVER_LOGGED_IN:
                    printf("Client: Server logged in\n\n");
                    break;
                case SERVER_SPECIFY_PASSWORD:
                    printf("Client: Server requires password\n\n");
                    return 1;
                case SERVER_NOT_AVAILABLE:
                    printf("Client: Server is not available\n");
                    closeConnections();
                    return -1;
                case SERVER_BAD_COMMAND:
                    printf("Client: This command is malformed (command type) -> %s\n",command);
                    printf("Help: See RFC 959 for right command usage\n");
                    closeConnections();
                    return -1;
                case SERVER_BAD_PARAMETERS:
                    printf("Client: This command is malformed (command parameters) -> %s\n",command);
                    printf("Help: See RFC 959 for right command usage\n");
                    closeConnections();
                    return -1;
                case SERVER_NOT_LOGGED_IN:
                    printf("Client: username,password or account name are wrong\n");
                    printf("Help: Change username to anonymous, or to right account.\n");
                    closeConnections();
                    return -1;
            }
            return 0;
        }
    }

    fprintf(stderr,"ERROR: Server code not expected (RFC959 standards)\n");
    return -1;
}


// Login
int login(char* username, char* password, char* ip, char* host, int port){
    // Connect to server
    if (!username || !password || !ip || !host){
        fprintf(stderr, "ERROR: NULL parameters\n");
        return -1;
    }

    printf("Client: Trying to establish connection to %s\n", host);
    controlFd = createConnection(ip, port);
    if (controlFd == -1){
        fprintf(stderr,"ERROR: Cannot establish connection\n");
        closeConnections();
        return -1;
    }

    char response[MAX_CONTROL_SIZE + 1];
    int code, status = serverResponse(response, &code);

    if (status == -1){
        fprintf(stderr, "ERROR: Server response failed\n");
        closeConnections();
        return -1;
    }

    printf("Server: %s", response);
    int possibleCodesConnect[3] = {120,220,421};
    int result;
    if ((result = processServerCode(code, possibleCodesConnect, NULL, response)) == -1){
        return -1;
    }
    printf("Client: Connection open on ip:%s, hostname:%s, port:%d\n\n",ip,host,port);

    // Credentials
    // User
    int commandLength = strlen(COMMAND_USER) + strlen(username) + 3;
    char commandUser[commandLength + 1];
    sprintf(commandUser, "%s %s\r\n", COMMAND_USER, username);

    int bytesSent = write(controlFd, commandUser, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        closeConnections();
        return -1;
    }
    printf("Client: %s", commandUser);

    status = serverResponse(response, &code);
    if (status == -1){
        fprintf(stderr, "ERROR: Server response failed\n");
        closeConnections();
        return -1;
    }

    printf("Server: %s", response);
    int possibleCodesUser[6] = {230, 530, 500, 501, 421, 331};
    if ((result = processServerCode(code, possibleCodesUser, commandUser, response)) == -1){
        return -1;
    }
    else if (result == 1){
        // Password
        commandLength = strlen(COMMAND_PASS) + strlen(password) + 3;
        char commandPass[commandLength + 1];
        sprintf(commandPass, "%s %s\r\n", COMMAND_PASS, password);

        bytesSent = write(controlFd, commandPass, commandLength);
        if (bytesSent != commandLength){
            fprintf(stderr, "ERROR: Number of bytes sent\n");
            closeConnections();
            return -1;
        }
        printf("Client: %s", commandPass);

        status = serverResponse(response, &code);
        if (status == -1){
            fprintf(stderr, "ERROR: Server response failed\n");
            closeConnections();
            return -1;
        }
        printf("Server: %s", response);
        int possibleCodesPass[7] = {230, 202, 530, 500, 501, 503, 421};
        if ((result = processServerCode(code, possibleCodesPass, commandPass, response)) == -1){
            return -1;
        }
    }
    return 0;
}



// Download
// FTP server - Find file (CWD)
// FTP server - Change to typecode format ('I' - binary , 'A' - Ascii) (TYPE)
// FTP server - Get file size (SIZE)
// FTP server - PASV
// FTP server - Download file (RETR)



// Logout
int logout(){
    // Quit
    int commandLength = strlen(COMMAND_QUIT);
    int bytesSent = write(controlFd, COMMAND_QUIT, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Number of bytes sent\n");
        closeConnections();
        return -1;
    }
    printf("Client: %s", COMMAND_QUIT);

    char response[MAX_CONTROL_SIZE + 1];
    int code, status = serverResponse(response, &code);
    if (status == -1){
        fprintf(stderr, "ERROR: Server response failed\n");
        closeConnections();
        return -1;
    }
    printf("Server: %s", response);

    int possibleCodesQuit[2] = {221, 500};
    int result;
    if ((result = processServerCode(code, possibleCodesQuit, COMMAND_QUIT, response)) == -1){
        return -1;
    }
    return 0;
}

