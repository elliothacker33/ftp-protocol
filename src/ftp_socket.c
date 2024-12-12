#include <fcntl.h>
#include "ftp_socket.h"

// File descriptor for control and data 
int controlFd = -1;
int dataFd = -1;
FILE* localFd;

int createConnection(const char* ip, const int port){

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = IPV4;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);    
    
    int fd = socket(IPV4, CONNECTION_TYPE, 0);
    if (fd < 0){
        fprintf(stderr, "ERROR: Socket creation failed\n");
        return ERROR_SOCKET_CREATION_FAILED;
    }

    if (connect(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "ERROR: Connection failed\n");
        return ERROR_CONNECTION_SERVER_FAILED;
    }

    return fd;
}

int closeConnection(int fd){

    if (fd != -1 && close(fd) == -1){
        fprintf(stderr, "ERROR: Closing control socket\n");
        return ERROR_CLOSING_SOCKET;
    }

    fd = -1;
    return SUCCESS;
}


int serverResponse(char *responseControl, int *code) {

    // Read byte
    char byte;

    // States
    ControlState stateC = STATE_WAIT_CODE;
    DataState stateD = STATE_WAIT_DATA;

    // Code and response
    int totalBytes = 0, digitsRead = 0;
    *code = 0;
    memset(responseControl, 0, MAX_CONTROL_SIZE + 1);

    // Select 
    int maxFd;
    fd_set readFdSet;
    struct timeval timeout;
    timeout.tv_sec = SECONDS;
    timeout.tv_usec = MILLISECONDS;
    int tries = 0;

    while ((stateC != STATE_STOP && controlFd != -1) || (stateD != STATE_FULL_FILE_READ && dataFd != -1)) {
        // Sets 
        FD_ZERO(&readFdSet);

        // Add controlFd to set
        if (controlFd != -1){
            FD_SET(controlFd, &readFdSet);
        }
        // Add dataFd to set
        if (dataFd != -1){
            FD_SET(dataFd, &readFdSet);
        }
        // Wait for data
        maxFd = (controlFd > dataFd) ? controlFd : dataFd;
        int ready = select(maxFd + 1, &readFdSet, NULL, NULL, &timeout);

        if (ready != -1) {
            tries = 0; // Reset tries

            // Read from control file descriptor
            if (controlFd != -1 && FD_ISSET(controlFd, &readFdSet) && stateC != STATE_STOP) {
                int bytesRead = read(controlFd, &byte, 1);
                if (bytesRead > 0) {
                    if (totalBytes >= MAX_CONTROL_SIZE) {
                        fprintf(stderr, "ERROR: Response exceeded max size\n");
                        return ERROR_EXCEEDED_MAX_SIZE;
                    }
                    responseControl[totalBytes++] = byte;

                    switch (stateC) {
                        case STATE_WAIT_CODE:
                            if (isdigit(byte)) {
                                digitsRead++;
                                *code = *code * 10 + (byte - '0');
                                if (digitsRead == 3){
                                    stateC = STATE_WAIT_SP;
                                }
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
                    return ERROR_SERVER_CLOSED_CONNECTION;
                } else {
                    fprintf(stderr, "ERROR: Reading from data connection\n");
                    return ERROR_READ_SOCKET_FAILED;
                }
            }
            
            // Read from data file descriptor
            if (dataFd != -1 && FD_ISSET(dataFd, &readFdSet) && stateD != STATE_FULL_FILE_READ) {
                int bytesRead = read(dataFd, &byte, 1);
                if (bytesRead > 0) {
                    // Handle data read from dataFd
                    if (fwrite(&byte, 1, bytesRead, localFd) != bytesRead) {
                        fprintf(stderr, "ERROR: Writing to local file\n");
                        return ERROR_WRITE_FILE_FAILED;
                    }
                } else if (bytesRead == 0) {
                    stateD = STATE_FULL_FILE_READ;
                } else {
                    fprintf(stderr, "ERROR: Reading from data connection\n");
                    return ERROR_READ_SOCKET_FAILED;
                }
            }
        } else {
            tries++;
            if (tries >= MAX_TIMEOUT_TRIES) {
                fprintf(stderr, "ERROR: Server response timeout\n");
                return ERROR_MAX_TIMEOUT;
            }
        }
    }

    return SUCCESS;
}

int processServerCode(const int code, const int* possibleCodes, const char* command, const char* response){
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
                        return ERROR_SERVER_CODE;
                    }
                    break;
                }
                case SERVER_DATA_CONNECTION_OPEN:
                    printf("Client: Data connection open\n");
                    break;
                case SERVER_OPENING_DATA_CONNECTION:
                    printf("Client: Opening data connection\n");
                    break;
                case SERVER_COMMAND_OK:
                    printf("Client: Command -> %s, successful\n\n", command);
                    break;
                case SERVER_COMMAND_NOT_IMPLEMENTED_SUPERFLUOUS:
                    printf("Client: Command -> %s, not implemented on this server at superfluous level\n", command);
                    return ERROR_SERVER_CODE;
                case SERVER_FILE_STATUS:
                    printf("Client: File status -> %s\n", response);
                    break;
                case SERVER_READY:
                    printf("Client: Server is ready\n");
                    break;
                case SERVER_QUIT:
                    printf("Client: Server quit\n");
                    break;
                case SERVER_DATA_CONNECTION_CLOSE:
                    printf("Client: Data connection closed\n");
                    break;
                case SERVER_ENTERING_PASSIVE_MODE:
                    printf("Client: Entering passive mode\n");
                    break;
                case SERVER_LOGGED_IN:
                    printf("Client: Server authentication done\n\n");
                    break;
                case SERVER_FILE_ACTION_OK:
                    printf("Client: File action successful\n");
                    break;
                case SERVER_SPECIFY_PASSWORD:
                    printf("Client: Server requires password\n\n");
                    return PASSWORD;
                case SERVER_NOT_AVAILABLE:
                    printf("Client: Server is not available\n");
                    return ERROR_SERVER_CODE;
                case SERVER_CANNOT_OPEN_DATA_CONNECTION:
                    printf("Client: Cannot open data connection\n");
                    return ERROR_SERVER_CODE;
                case SERVER_DATA_CONNECTION_CLOSED_TRANSFER_ABORTED:
                    printf("Client: Data connection closed, transfer aborted\n");
                    return ERROR_SERVER_CODE;
                case SERVER_REQUESTED_FILE_ACTION_ABORTED:
                    printf("Client: Requested file action aborted\n");
                    return ERROR_SERVER_CODE;
                case SERVER_REQUESTED_ACTION_ABORTED:
                    printf("Client: Requested action aborted\n");
                    return ERROR_SERVER_CODE;
                case SERVER_BAD_COMMAND:
                    printf("Client: This command is malformed (command type) -> %s\n",command);
                    printf("Help: See RFC 959 for right command usage\n");
                    return ERROR_SERVER_CODE;
                case SERVER_BAD_PARAMETERS:
                    printf("Client: This command is malformed (command parameters) -> %s\n",command);
                    printf("Help: See RFC 959 for right command usage\n");
                    return ERROR_SERVER_CODE;
                case SERVER_COMMAND_NOT_IMPLEMENTED:
                    printf("Client: Command -> %s, is not implemented on this server\n",command);
                    return ERROR_SERVER_CODE;
                case SERVER_COMMAND_NOT_IMPLEMENTED_FOR_PARAMETER:
                    printf("Client: Command -> %s, is not implemented for this parameter\n",command);
                    return ERROR_SERVER_CODE;
                case SERVER_NOT_LOGGED_IN:
                    printf("Client: username,password or account name are wrong\n");
                    printf("Help: Change username to anonymous, or to right account.\n");
                    return ERROR_SERVER_CODE;
                case SERVER_FILE_UNAVAILABLE:
                    printf("Client: File is unavailable\n");
                    return ERROR_SERVER_CODE;
            }
            return SUCCESS;
        }
    }

    fprintf(stderr,"ERROR: Server code not expected (RFC959 standards)\n");
    return ERROR_SERVER_CODE;
}

int openControlConnection(const char* ip, const char* host, const int port){

    printf("Client: Trying to establish connection to %s...\n", host);
    controlFd = createConnection(ip, port);

    if (controlFd < 0){
        fprintf(stderr,"ERROR: Cannot establish connection to server\n");
        return ERROR_CONNECTION_SERVER_FAILED;
    }

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    int status = serverResponse(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    int possibleCodesConnect[3] = {
        SERVER_WAIT_N,
        SERVER_READY,
        SERVER_NOT_AVAILABLE
    };

    int result = processServerCode(code, possibleCodesConnect, NULL, response);
    if (result != SUCCESS){
        return ERROR_SERVER_CODE;
    }

    printf("Client: Connection open on ip:%s, hostname:%s, port:%d\n\n",ip,host,port);
    return SUCCESS;
}

int authenticateCredentials(const char* username, const char* password){

    // (USER)
    int commandLength = strlen(COMMAND_USER) + strlen(username) + 3; // 3 - SPACE + \r + \n
    char commandUser[commandLength + 1]; 
    sprintf(commandUser, "%s %s\r\n", COMMAND_USER, username);

    int bytesSent = write(controlFd, commandUser, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandUser);

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    int status = serverResponse(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    int possibleCodesUser[6] = {
        SERVER_LOGGED_IN,
        SERVER_NOT_LOGGED_IN,
        SERVER_BAD_COMMAND,
        SERVER_BAD_PARAMETERS,
        SERVER_NOT_AVAILABLE,
        SERVER_SPECIFY_PASSWORD
    };

    int result = processServerCode(code, possibleCodesUser, commandUser, response);
    if (result == SUCCESS){
        printf("Client: Authentication successful (Only USER authentication)\n");
        return SUCCESS;
    }
    else if (result == PASSWORD){

        // (PASS)
        commandLength = strlen(COMMAND_PASS) + strlen(password) + 3;
        char commandPass[commandLength + 1];
        sprintf(commandPass, "%s %s\r\n", COMMAND_PASS, password);

        int bytesSent = write(controlFd, commandPass, commandLength);
        if (bytesSent != commandLength){
            fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
            return ERROR_WRITE_SOCKET_FAILED;
        }
        printf("Client: %s", commandPass);

        status = serverResponse(response, &code);
        if (status != SUCCESS){
            fprintf(stderr, "ERROR: Server response failed\n");
            return ERROR_SERVER_RESPONSE_FAILED;
        }
        printf("Server: %s", response);

        int possibleCodesPass[7] = {
            SERVER_LOGGED_IN,
            SERVER_COMMAND_NOT_IMPLEMENTED_SUPERFLUOUS,
            SERVER_NOT_LOGGED_IN,
            SERVER_BAD_COMMAND,
            SERVER_BAD_PARAMETERS,
            SERVER_BAD_SEQUENCE_COMMANDS,
            SERVER_NOT_AVAILABLE
        };
        
        result = processServerCode(code, possibleCodesPass, commandPass, response);
        if (result != SUCCESS){
            return ERROR_SERVER_CODE;
        }
    }
    else{
        return ERROR_SERVER_CODE;
    }

    return SUCCESS;
}

// Login
int login(const char* username, const char* password, const char* ip, const char* host, const int port){
 
      // Connect to server
    if (openControlConnection(ip, host, port) != SUCCESS){
        closeConnection(controlFd);
        return ERROR_OPEN_CONTROL_CONNECTION;
    };

    // Authenticate (USER,PASS)
    if (authenticateCredentials(username, password) != SUCCESS){
        closeConnection(controlFd);
        return ERROR_AUTHENTICATION_FAILED;
    }

    return SUCCESS;
}




int cwd(char directories[21][256]){
    for (int i = 0; directories[i][0] != '\0'; i++) {

        int commandLength = strlen(COMMAND_CWD) + strlen(directories[i]) + 3;
        char commandCwd[commandLength + 1];
        sprintf(commandCwd, "%s %s\r\n", COMMAND_CWD, directories[i]);

        int bytesSent = write(controlFd, commandCwd, commandLength);
        if (bytesSent != commandLength) {
            fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
            return ERROR_WRITE_SOCKET_FAILED;
        }
        printf("Client: %s", commandCwd);

        char response[MAX_CONTROL_SIZE + 1];
        int code;

        int status = serverResponse(response, &code);
        if (status != SUCCESS) {
            fprintf(stderr, "ERROR: Server response failed\n");
            return ERROR_SERVER_RESPONSE_FAILED;
        }
        printf("Server: %s", response);

        int possibleCodesCwd[7] = {
            SERVER_FILE_ACTION_OK,
            SERVER_BAD_COMMAND,
            SERVER_BAD_PARAMETERS,
            SERVER_COMMAND_NOT_IMPLEMENTED,
            SERVER_NOT_AVAILABLE,
            SERVER_NOT_LOGGED_IN,
            SERVER_FILE_UNAVAILABLE
        };
    
        int result;
        if ((result = processServerCode(code, possibleCodesCwd, commandCwd, response)) != SUCCESS) {
            return ERROR_SERVER_CODE;
        }
        printf("\n");
    }
    return SUCCESS;
}


int getFileSize(const char* filename, int* fileSize) {
    int commandLength = strlen(COMMAND_SIZE) + strlen(filename) + 3;
    char commandSize[commandLength + 1];
    sprintf(commandSize, "%s %s\r\n", COMMAND_SIZE, filename);

    int bytesSent = write(controlFd, commandSize, commandLength);
    if (bytesSent != commandLength) {
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandSize);

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    int status = serverResponse(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    int possibleCodesSize[1] = { SERVER_FILE_STATUS };
    int result = processServerCode(code, possibleCodesSize, commandSize, response);
    if (result != SUCCESS) {
        return ERROR_SERVER_CODE;
    }

    if (sscanf(response, "%*d %d", fileSize) != 1) {
        fprintf(stderr, "ERROR: Failed to parse file size\n");
        return ERROR_PARSE;
    }

    return SUCCESS;
}

int changeType(const char typecode){
    if (typecode != 'i' && typecode != 'I'){
        fprintf(stderr, "ERROR: Invalid typecode\n");
        return ERROR_INVALID_TYPECODE;
    }
    int commandLength = strlen(COMMAND_TYPE) + 4;
    char commandType[commandLength + 1];
    sprintf(commandType, "%s %c\r\n", COMMAND_TYPE, typecode);

    int bytesSent = write(controlFd, commandType, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandType);

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    int status = serverResponse(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    int possibleCodesType[6] = {
        SERVER_COMMAND_OK,
        SERVER_BAD_COMMAND,
        SERVER_BAD_PARAMETERS,
        SERVER_COMMAND_NOT_IMPLEMENTED_FOR_PARAMETER,
        SERVER_NOT_AVAILABLE,
        SERVER_NOT_LOGGED_IN
    };
    int result;
    if ((result = processServerCode(code, possibleCodesType, commandType, response)) != SUCCESS){
        return ERROR_SERVER_CODE;
    }
    return SUCCESS;
}

int enterPassiveMode(char* ip, int* port) {
    int commandLength = strlen(COMMAND_PASV) + 3;
    char commandPasv[commandLength + 1];
    sprintf(commandPasv, "%s\r\n", COMMAND_PASV);

    // Send PASV command to the server
    int bytesSent = write(controlFd, commandPasv, commandLength);
    if (bytesSent != commandLength) {
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }

    printf("Client: %s", commandPasv);

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    // Get server response
    int status = serverResponse(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Check the server response code
    if (code != SERVER_ENTERING_PASSIVE_MODE) {
        fprintf(stderr, "ERROR: Unexpected server response code: %d\n", code);
        return ERROR_SERVER_CODE;
    }

    int h1, h2, h3, h4, p1, p2;
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "ERROR: Failed to parse IP and port from server response\n");
        return ERROR_PARSE;
    }

    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4); // IPv4 address
    *port = p1 * 256 + p2; // Port

    printf("Parsed IP: %s, Port: %d\n", ip, *port);

    return SUCCESS;
}

int downloadFile(const char* filename, const char* localPath) {
    int commandLength = strlen(COMMAND_RETR) + strlen(filename) + 3;
    char commandRetr[commandLength + 1];
    sprintf(commandRetr, "%s %s\r\n", COMMAND_RETR, filename);

    int bytesSent = write(controlFd, commandRetr, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandRetr);

    localFd = fopen(localPath, "wb");
    if (!localFd) {
        fprintf(stderr, "ERROR: Cannot open local file\n");
        return ERROR_OPEN_FILE;
    }

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    int status = serverResponse(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        fclose(localFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }

    printf("Server: %s", response);
    if (code != SERVER_OPENING_DATA_CONNECTION){
        fprintf(stderr, "ERROR: Unexpected server response code: %d\n", code);
        fclose(localFd);
        return ERROR_SERVER_CODE;
    }

    status = serverResponse(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        fclose(localFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    if (code != SERVER_DATA_CONNECTION_CLOSE){
        fprintf(stderr, "ERROR: Unexpected server response code: %d\n", code);
        fclose(localFd);
        return ERROR_SERVER_CODE;
    }

    fclose(localFd);
    printf("File downloaded successfully: %s/%s\n", localPath,filename);

    return SUCCESS;
}

int download(char directories[21][256], const char* filename, const char typecode){

    // Change directories
    if (cwd(directories) != SUCCESS){
        closeConnection(controlFd);
        return ERROR_CWD_FAILED;
    }

    // Type code (Only Image supported)
    if (changeType(typecode)){
        closeConnection(controlFd);
        return ERROR_CHANGE_TYPE_FAILED;
    }

    // Get file size
    int fileSize;
    if (getFileSize(filename, &fileSize) != SUCCESS){
        closeConnection(controlFd);
        return ERROR_GET_SIZE_FAILED;
    }

    // Enter passive mode
    char ip[256];
    int port;
    if (enterPassiveMode(ip, &port) != SUCCESS){
        closeConnection(controlFd);
        return ERROR_ENTER_PASSIVE_MODE_FAILED;
    }

    // Open data connection
    dataFd = createConnection(ip,port);
    if (controlFd < 0){
        fprintf(stderr,"ERROR: Cannot establish connection to server\n");
        closeConnection(dataFd);
        closeConnection(controlFd);
        return ERROR_CONNECTION_SERVER_FAILED;
    }
    printf("Client: Connection open on ip:%s, port:%d\n\n", ip, port);


    // Download file
    char* localPath = "../downloads";
    if (downloadFile(filename,localPath) !=  SUCCESS){
        closeConnection(dataFd);
        closeConnection(controlFd);
        return ERROR_DOWNLOAD_FILE_FAILED;
    }

    return closeConnection(dataFd);
}

// Logout
int logout(){

    // (QUIT)
    int commandLength = strlen(COMMAND_QUIT);
    int bytesSent = write(controlFd, COMMAND_QUIT, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        closeConnection(controlFd);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", COMMAND_QUIT);

    char response[MAX_CONTROL_SIZE + 1];
    int code;

    int status = serverResponse(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        closeConnection(controlFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    int possibleCodesQuit[2] = {
        SERVER_QUIT, 
        SERVER_BAD_COMMAND,
    };

    int result = processServerCode(code, possibleCodesQuit, COMMAND_QUIT, response);
    if (result != SUCCESS){
        return ERROR_SERVER_CODE;
    }

    return closeConnection(controlFd);
}

