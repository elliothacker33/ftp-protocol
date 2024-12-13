#include <fcntl.h>
#include "ftp_socket.h"

// FILE DESCRIPTORS
int controlFd = -1;
int dataFd = -1;
FILE* localFd;

// SOCKETS
int openSocket(const char* ip, const int port, struct sockaddr_in* server_addr){
    
    // Socket settings (server address)
    bzero(server_addr, sizeof(*server_addr));
    server_addr->sin_family = IPV4;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(ip);    
    
    // Open socket
    int fd = socket(IPV4, CONNECTION_TYPE, 0);
    if (fd < 0){
        fprintf(stderr, "ERROR: Socket creation failed\n");
        return ERROR_OPEN_SOCKET_FAILED;
    }

    return fd;

}

int closeSocket(int fd){

    // Close socket
    if (fd >= 0 && close(fd) == -1){
        fprintf(stderr, "ERROR: Closing control socket\n");
        return ERROR_CLOSE_SOCKET_FAILED;
    }

    fd = -1;
    return SUCCESS;

}

// SERVER RESPONSE (RECEIVE + PROCESSING)
int serverResponseControl(char *responseControl, int *code){

    char byte;
    ControlState stateC = STATE_WAIT_CODE;

    int totalBytes = 0, digitsRead = 0;
    *code = 0;
    memset(responseControl, 0, MAX_CONTROL_SIZE + 1);

    // Control response - <CODE> <SP> ... <CRLF> - Stopping condition
    while (stateC != STATE_STOP) {

        int bytesRead = read(controlFd, &byte, 1);
        // Handle control read from controlFd
        if (bytesRead > 0) {
            if (totalBytes >= MAX_CONTROL_SIZE) {
                fprintf(stderr, "ERROR: Response exceeded max size\n");
                return ERROR_EXCEEDED_MAX_ARRAY_SIZE;
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
        }
        else if (bytesRead == 0) {
            fprintf(stderr, "ERROR: Server closed connection\n");
            return ERROR_SERVER_CLOSED_CONNECTION;
        } 
        else{
            fprintf(stderr, "ERROR: Reading from data connection\n");
            return ERROR_READ_SOCKET_FAILED;
        }

    }
    return SUCCESS;
}

int serverResponseData(){

    DataState stateD = STATE_WAIT_DATA;
    char byte[DATA_CHUNK_SIZE];

    // Data response - Read until server closes data connection - Stopping condition
    while (stateD != STATE_FULL_FILE_READ){

        int bytesRead = read(dataFd, byte, DATA_CHUNK_SIZE);    
        if (bytesRead > 0) {
            // Handle data read from dataFd
            if (fwrite(byte, 1, bytesRead, localFd) != bytesRead) {
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
    return closeSocket(dataFd);

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
                        return ERROR_SERVER_CODE_EXPECTED;
                    }
                    break;
                }
                case SERVER_DATA_CONNECTION_OPEN:
                    printf("Client: Data connection open\n");
                    break;
                case SERVER_OPENING_DATA_CONNECTION:
                    printf("Client: File transfer started\n");
                    break;
                case SERVER_COMMAND_OK:
                    printf("Client: Command -> %s, successful\n\n", command);
                    break;
                case SERVER_COMMAND_NOT_IMPLEMENTED_SUPERFLUOUS:
                    printf("Client: Command -> %s, not implemented on this server at superfluous level\n", command);
                    return ERROR_SERVER_CODE_EXPECTED;
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
                    return CHANGE_SERVER_REDIRECT_PASSWORD;
                case SERVER_NOT_AVAILABLE:
                    printf("Client: Server is not available\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_CANNOT_OPEN_DATA_CONNECTION:
                    printf("Client: Cannot open data connection\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_DATA_CONNECTION_CLOSED_TRANSFER_ABORTED:
                    printf("Client: Data connection closed, transfer aborted\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_REQUESTED_FILE_ACTION_ABORTED:
                    printf("Client: Requested file action aborted\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_REQUESTED_ACTION_ABORTED:
                    printf("Client: Requested action aborted\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_BAD_COMMAND:
                    printf("Client: This command is malformed (command type) -> %s\n",command);
                    printf("Help: See RFC 959 for right command usage\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_BAD_PARAMETERS:
                    printf("Client: This command is malformed (command parameters) -> %s\n",command);
                    printf("Help: See RFC 959 for right command usage\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_COMMAND_NOT_IMPLEMENTED:
                    printf("Client: Command -> %s, is not implemented on this server\n",command);
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_COMMAND_NOT_IMPLEMENTED_FOR_PARAMETER:
                    printf("Client: Command -> %s, is not implemented for this parameter\n",command);
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_NOT_LOGGED_IN:
                    printf("Client: username,password or account name are wrong\n");
                    printf("Help: Change username to anonymous, or to right account.\n");
                    return ERROR_SERVER_CODE_EXPECTED;
                case SERVER_FILE_UNAVAILABLE:
                    printf("Client: File is unavailable\n");
                    return ERROR_SERVER_CODE_EXPECTED;
            }
            return SUCCESS;
        }
    }

    fprintf(stderr,"ERROR: Server code not expected (RFC959 standards)\n");
    return ERROR_SERVER_CODE_NOT_EXPECTED;

}

// OPEN CONNECTIONS
int openControlConnection(const char* ip, const char* host, const int port){

    // Open control Socket
    struct sockaddr_in server_addr;
    controlFd = openSocket(ip, port, &server_addr);
    if (controlFd < 0){
        fprintf(stderr,"ERROR: Cannot open control socket\n");
        return ERROR_OPEN_SOCKET;
    }

    // Create control connection
    printf("Client: Trying to establish a control connection on ip: %s, port: %d...\n", ip, port);
    if (connect(controlFd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "ERROR: Connection failed\n");
        return ERROR_CONNECTION_SERVER_FAILED;
    }

    // Read control response for connection (220)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Process response from server
    int possibleCodesConnect[3] = {
        SERVER_WAIT_N,
        SERVER_READY,
        SERVER_NOT_AVAILABLE
    };
    int result = processServerCode(code, possibleCodesConnect, NULL, response);
    if (result != SUCCESS){
        return ERROR_SERVER_CODE;
    }

    printf("Client: Control connection open on ip:%s, hostname:%s, port:%d\n\n",ip,host,port);
    return SUCCESS;

}

int openDataConnection(const char* ip, const int port){

    // Open data socket
    struct sockaddr_in server_addr;
    dataFd = openSocket(ip, port, &server_addr);
    if (dataFd < 0){
        fprintf(stderr,"ERROR: Cannot open data socket\n");
        return ERROR_OPEN_SOCKET;
    }

    // Create control connection
    printf("Client: Trying to establish a data connection on ip: %s, port: %d...\n", ip, port);
    if (connect(dataFd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "ERROR: Connection failed\n");
        return ERROR_CONNECTION_SERVER_FAILED;
    }

    // Shutdown write connection
    shutdown(dataFd, SHUT_WR);

    printf("Client: Data connection open on ip:%s, port:%d\n\n",ip, port);
    return SUCCESS;

}

// LOGIN (OPEN CONNECTION + AUTHENTICATION)
int authenticateCredentials(const char* username, const char* password){

    // Build (USER) command
    int commandLength = strlen(COMMAND_USER) + strlen(username) + 3; // 3 - SPACE + \r + \n
    char commandUser[commandLength + 1]; 
    sprintf(commandUser, "%s %s\r\n", COMMAND_USER, username);

    // Send (USER) command
    int bytesSent = write(controlFd, commandUser, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandUser);

    // Read response for authentication (230/331)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Process response from server
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
    else if (result == CHANGE_SERVER_REDIRECT_PASSWORD){

        // Build (PASS) command
        commandLength = strlen(COMMAND_PASS) + strlen(password) + 3; // 3 - SPACE + \r + \n
        char commandPass[commandLength + 1];
        sprintf(commandPass, "%s %s\r\n", COMMAND_PASS, password);

        // Send (PASS) command
        int bytesSent = write(controlFd, commandPass, commandLength);
        if (bytesSent != commandLength){
            fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
            return ERROR_WRITE_SOCKET_FAILED;
        }
        printf("Client: %s", commandPass);

        // Read response for authentication (230)
        status = serverResponseControl(response, &code);
        if (status != SUCCESS){
            fprintf(stderr, "ERROR: Server response failed\n");
            return ERROR_SERVER_RESPONSE_FAILED;
        }
        printf("Server: %s", response);
        
        // Process response from server
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


int login(const char* username, const char* password, const char* ip, const char* host, const int port){
 
      // Connect to server
    if (openControlConnection(ip, host, port) != SUCCESS){
        closeSocket(controlFd);
        return ERROR_OPEN_CONTROL_CONNECTION;
    };

    // Authenticate (USER + [PASS]?)
    if (authenticateCredentials(username, password) != SUCCESS){
        closeSocket(controlFd);
        return ERROR_AUTHENTICATION_FAILED;
    }

    return SUCCESS;

}

// CHANGE DIRECTORY
int cwd(char directories[URL_MAX_CWD + 1][URL_FIELD_MAX_LENGTH + 1]){

    for (int i = 0; directories[i][0] != '\0'; i++) {
        
        // Build (CWD) command
        int commandLength = strlen(COMMAND_CWD) + strlen(directories[i]) + 3;
        char commandCwd[commandLength + 1];
        sprintf(commandCwd, "%s %s\r\n", COMMAND_CWD, directories[i]);

        // Send (CWD) command
        int bytesSent = write(controlFd, commandCwd, commandLength);
        if (bytesSent != commandLength) {
            fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
            return ERROR_WRITE_SOCKET_FAILED;
        }
        printf("Client: %s", commandCwd);

        // Read response for changing directory (250)
        char response[MAX_CONTROL_SIZE + 1];
        int code,status;
        status = serverResponseControl(response, &code);
        if (status != SUCCESS) {
            fprintf(stderr, "ERROR: Server response failed\n");
            return ERROR_SERVER_RESPONSE_FAILED;
        }
        printf("Server: %s", response);

        // Process response from server
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

// CHANGE TRANSFER TYPE CODE
int changeType(const char typecode){

    // Build (TYPE) command
    int commandLength = strlen(COMMAND_TYPE) + 4;
    char commandType[commandLength + 1];
    sprintf(commandType, "%s %c\r\n", COMMAND_TYPE, typecode);

    // Send (TYPE) command
    int bytesSent = write(controlFd, commandType, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandType);

    // Read response for changing type (200)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Process response from server
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

// GET LOCAL FILE SIZE
long getLocalFileSize(FILE* fptr){

    if ((fseek(fptr, 0, SEEK_END)) == -1) {
        return ERROR_GET_FILE_SIZE;
    }

    long fileSize = ftell(fptr);
    if (fileSize == -1){
        return ERROR_GET_FILE_SIZE;
    }
    rewind(fptr);
    return fileSize;

}

// GET SIZE OF THE REQUESTED FILE
int getFileSize(const char* filename, int* fileSize){

    // Build (SIZE) command
    int commandLength = strlen(COMMAND_SIZE) + strlen(filename) + 3;
    char commandSize[commandLength + 1];
    sprintf(commandSize, "%s %s\r\n", COMMAND_SIZE, filename);

    // Send (SIZE) command
    int bytesSent = write(controlFd, commandSize, commandLength);
    if (bytesSent != commandLength) {
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandSize);

    // Read response for getting file size (213)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Process response from server
    int possibleCodesSize[1] = { SERVER_FILE_STATUS };
    int result = processServerCode(code, possibleCodesSize, commandSize, response);
    if (result != SUCCESS) {
        return ERROR_SERVER_CODE;
    }

    // Parse file size from the response
    if (sscanf(response, "%*d %d", fileSize) != 1) {
        fprintf(stderr, "ERROR: Failed to parse file size\n");
        return ERROR_PARSE;
    }

    return SUCCESS;

}

// ENTER PASSIVE MODE
int enterPassiveMode(char* ip, int* port) {

    // Build (PASV) command
    int commandLength = strlen(COMMAND_PASV) + 2;
    char commandPasv[commandLength + 1];
    sprintf(commandPasv, "%s\r\n", COMMAND_PASV);

    // Send (PASV) command 
    int bytesSent = write(controlFd, commandPasv, commandLength);
    if (bytesSent != commandLength) {
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandPasv);

    // Read response for entering passive mode (227)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Check the server response code
    int possibleCodesPassive[6] = { 
        SERVER_ENTERING_PASSIVE_MODE,
        SERVER_BAD_COMMAND,
        SERVER_BAD_PARAMETERS,
        SERVER_COMMAND_NOT_IMPLEMENTED,
        SERVER_NOT_AVAILABLE,
        SERVER_NOT_LOGGED_IN};

    int result = processServerCode(code, possibleCodesPassive, commandPasv, response);
    if (result != SUCCESS) {
        return ERROR_SERVER_CODE;
    }

    // Parse IP and port from the server response
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

// RETRIEVE FILE
int downloadFile(const char* filename, const char* localPath, int* localFileSize) {

    // Build (RETR) command
    int commandLength = strlen(COMMAND_RETR) + strlen(filename) + 3;
    char commandRetr[commandLength + 1];
    sprintf(commandRetr, "%s %s\r\n", COMMAND_RETR, filename);

    // Send (RETR) command
    int bytesSent = write(controlFd, commandRetr, commandLength);
    if (bytesSent != commandLength){
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandRetr);
    
    // Open file
    localFd = fopen(localPath, "wb");
    if (!localFd) {
        fprintf(stderr, "ERROR: Cannot open local file\n");
        return ERROR_OPEN_FILE;
    }

    // Read response for downloading file (150)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        fclose(localFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Check the server response code
    int possibleCodesRetr1[11] = { 
        SERVER_DATA_CONNECTION_OPEN,
        SERVER_OPENING_DATA_CONNECTION,
        SERVER_CANNOT_OPEN_DATA_CONNECTION,
        SERVER_DATA_CONNECTION_CLOSED_TRANSFER_ABORTED,
        SERVER_REQUESTED_FILE_ACTION_ABORTED,
        SERVER_REQUESTED_ACTION_ABORTED,
        SERVER_FILE_UNAVAILABLE,
        SERVER_BAD_COMMAND,
        SERVER_BAD_PARAMETERS,
        SERVER_NOT_LOGGED_IN,
        SERVER_NOT_AVAILABLE};

    int result = processServerCode(code, possibleCodesRetr1, commandRetr, response);
    if (result != SUCCESS) {
        return ERROR_SERVER_CODE;
    }

    // Download file 
    status = serverResponseData();
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Failed to receive file data\n");
        fclose(localFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }

    // File read successfully
    status = serverResponseControl(response, &code);
    if (status != SUCCESS) {
        fprintf(stderr, "ERROR: Server response failed\n");
        fclose(localFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Check the server response code
    int possibleCodesRetr2[9] = { 
        SERVER_DATA_CONNECTION_CLOSE,
        SERVER_DATA_CONNECTION_CLOSED_TRANSFER_ABORTED,
        SERVER_REQUESTED_FILE_ACTION_ABORTED,
        SERVER_REQUESTED_ACTION_ABORTED,
        SERVER_FILE_UNAVAILABLE,
        SERVER_BAD_COMMAND,
        SERVER_BAD_PARAMETERS,
        SERVER_NOT_LOGGED_IN,
        SERVER_NOT_AVAILABLE
        };

    result = processServerCode(code, possibleCodesRetr2, commandRetr, response);
    if (result != SUCCESS) {
        return ERROR_SERVER_CODE;
    }

    *localFileSize = getLocalFileSize(localFd);
    if (*localFileSize < 0){
        fprintf(stderr, "ERROR: Failed to get local file size\n");
        fclose(localFd);
        return ERROR_GET_FILE_SIZE;
    }

    if (fclose(localFd) != SUCCESS){
        return ERROR_CLOSE_FILE;
    }
    printf("File downloaded successfully: %s\n", localPath);

    return SUCCESS;

}

// DOwNLOAD FILE
int download(char directories[URL_MAX_CWD + 1][URL_FIELD_MAX_LENGTH + 1], const char* filename, const char typecode){

    // Change directories
    if (cwd(directories) != SUCCESS){
        closeSocket(controlFd);
        return ERROR_CWD;
    }

    // Type code (Only Image supported)
    if (changeType(typecode)){
        closeSocket(controlFd);
        return ERROR_CHANGE_TYPE;
    }

    // Get file size
    int fileSize;
    if (getFileSize(filename, &fileSize) != SUCCESS){
        closeSocket(controlFd);
        return ERROR_GET_FILE_SIZE;
    }

    // Enter passive mode
    char ip[URL_FIELD_MAX_LENGTH + 1];
    int port = 0;
    if (enterPassiveMode(ip, &port) != SUCCESS){
        closeSocket(controlFd);
        return ERROR_ENTER_PASSIVE_MODE;
    }

    // Open data connection
    if (openDataConnection(ip, port) != SUCCESS){
        closeSocket(controlFd);
        return ERROR_OPEN_DATA_CONNECTION;
    }

    // Download file
    int localFileSize;
    char localPath[LOCAL_PATH_MAX_LENGTH];
    sprintf(localPath, "downloads/%s", filename);
    if (downloadFile(filename, localPath, &localFileSize) !=  SUCCESS){
        closeSocket(dataFd);
        closeSocket(controlFd);
        return ERROR_DOWNLOAD_FILE;
    }

    // File size integrity check
    if (localFileSize != fileSize){
        fprintf(stderr, "ERROR: File size does not match\n");
        closeSocket(controlFd);
        return ERROR_FILE_SIZE_MISMATCH;
    }
    printf("File size on the server matches the local file size\n\n");

    return SUCCESS;

}

// Logout
int logout(){

    // Build (QUIT) command
    int commandLength = strlen(COMMAND_QUIT) + 2; // 2 - \r + \n
    char commandQuit[commandLength + 1];
    sprintf(commandQuit, "%s\r\n", COMMAND_QUIT);

    // Send (PASV) command 
    int bytesSent = write(controlFd, commandQuit, commandLength);
    if (bytesSent != commandLength) {
        fprintf(stderr, "ERROR: Sent %d bytes, instead of %d\n", bytesSent, commandLength);
        return ERROR_WRITE_SOCKET_FAILED;
    }
    printf("Client: %s", commandQuit);

    // Read response for logging out (221)
    char response[MAX_CONTROL_SIZE + 1];
    int code, status;
    status = serverResponseControl(response, &code);
    if (status != SUCCESS){
        fprintf(stderr, "ERROR: Server response failed\n");
        closeSocket(controlFd);
        return ERROR_SERVER_RESPONSE_FAILED;
    }
    printf("Server: %s", response);

    // Check the server response code
    int possibleCodesQuit[2] = {
        SERVER_QUIT, 
        SERVER_BAD_COMMAND};

    int result = processServerCode(code, possibleCodesQuit, COMMAND_QUIT, response);
    if (result != SUCCESS){
        return ERROR_SERVER_CODE;
    }

    return closeSocket(controlFd);

}

