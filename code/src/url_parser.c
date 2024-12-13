#include "url_parser.h"

int decodePercent(char* parameter){
    int length = strlen(parameter);
    for (int i = 0; i < length; i++) {
        if (parameter[i] == '%') {
            if (i + 2 < length && isxdigit(parameter[i + 1]) && isxdigit(parameter[i + 2])) {
                char hex[3] = { parameter[i + 1], parameter[i + 2], '\0' }; 
                char eqChar = (char)strtol(hex, NULL, 16);
                parameter[i] = eqChar; 

                if (memmove(&parameter[i + 1], &parameter[i + 3], length - i - 2) == NULL){
                    fprintf(stderr,"ERROR: Memory allocation failed");
                    return ERROR_MEMORY_ALLOCATION;
                }
                length -= 2; 
            }
        }
    }
    return SUCCESS;
}

int ipAndHostChecker(char* hostname, char* ip){
    
    char* dnsResult = dnsLookup(hostname);
    if (dnsResult) {
        // Ip = dnsResult
        memcpy(ip, dnsResult, strlen(dnsResult));
    } 
    else {
        if (isdigit(hostname[0])) {
            char* reverseDnsResult = reverseDnsLookup(hostname);
            if (reverseDnsResult) {
                // Ip = hostname
                memcpy(ip, hostname, strlen(hostname));
                // Hostname = reverseDnsResult
                memcpy(hostname, reverseDnsResult, strlen(reverseDnsResult));

            } else {
                fprintf(stderr,"ERROR: Invalid host name or IP\n");
                return ERROR_INVALID_HOST_NANE;
            }
        }
        else{
            fprintf(stderr,"ERROR: Invalid host name or IP\n");
            return ERROR_INVALID_HOST_NANE;
        }
    }
    return SUCCESS;
}

int ftpUrlParser(const char* url, FTP_Parameters* parameters){

    // Url prefix
    if (strncmp(url,"ftp://",FTP_PREFIX_SIZE) != 0){
        fprintf(stderr,"ERROR: Invalid url prefix"); 
        return ERROR_INVALID_URL_FORMAT;
    }
    url += FTP_PREFIX_SIZE;

    // Username and password 
    char* posArr = strchr(url,'@');
    if (posArr){

        char* posColon = strchr(url, ':');
        int usernameLength, passwordLength;

        if (posColon && posColon < posArr){    
            usernameLength = posColon - url;
            passwordLength = posArr - posColon - 1;
        }
        else if (!posColon || (posColon && posColon > posArr)){
            usernameLength = posArr - url;
            passwordLength = 0;
        }
        else{
            fprintf(stderr,"ERROR: Invalid url format\n");
            return ERROR_INVALID_URL_FORMAT;
        }

        // Case 1: ftp://username:password@... or ftp://username@... 
        // Username
        if (usernameLength > 0 && usernameLength <= URL_FIELD_MAX_LENGTH){
            memcpy(parameters->username, url, usernameLength);

            // Prohibited chars
            if (strchr(parameters->username, '/')) {
                fprintf(stderr, "ERROR: Username contains '/'\n");
                return ERROR_INVALID_URL_FORMAT;
            }
            
            // Special chars
            if (strrchr(parameters->username, '%')){
                if (decodePercent(parameters->username) == -1){
                    return ERROR_DECODE;
                }
            }
        }
        else if (usernameLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: Invalid username size\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        }

        // Password
        if (passwordLength > 0 && passwordLength <= URL_FIELD_MAX_LENGTH){
            memcpy(parameters->password,posColon + 1,passwordLength);
            
            // Prohibited chars
            if (strchr(parameters->password, '/') || strchr(parameters->password, ':')) {
                fprintf(stderr, "ERROR: Username contains ':' or '/'\n");
                return ERROR_INVALID_URL_FORMAT;
            }

            // Special chars
            if (strrchr(parameters->password, '%')){
                if (decodePercent(parameters->password) == -1){
                    return ERROR_DECODE;
                }
            }
        }
        else if (passwordLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: Invalid password size\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        }

        url = posArr + 1;
    }
    else{
        // Case 2: ftp://ftp.up.pt......
        int anonymousUserLength = sizeof(USER_ANONYMOUS), anonymousPasswordLength = sizeof(PASS_ANONYMOUS);

        if (anonymousUserLength > 0 && anonymousUserLength <= URL_FIELD_MAX_LENGTH && anonymousPasswordLength > 0 && anonymousPasswordLength <= URL_FIELD_MAX_LENGTH){
            memcpy(parameters->username, USER_ANONYMOUS, anonymousUserLength);
            memcpy(parameters->password, PASS_ANONYMOUS, anonymousPasswordLength);
        }
        else if (anonymousUserLength > URL_FIELD_MAX_LENGTH || anonymousPasswordLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr, "ERROR: Invalid username or password size\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        }
    }


    // Host and Port logic 
    char* posSlash = strchr(url, '/');
    char* posColon = strchr(url,':');
    int hostNameLength;
    char portToken;

    if ((posSlash && posColon && posColon < posSlash) || (!posSlash && posColon && posColon < posSlash)){
        hostNameLength = posColon - url; 
        portToken = posSlash ? '/' : '\0';
    }
    else if (posSlash && (!posColon || (posColon && posColon > posSlash))){
        hostNameLength = posSlash - url;
    }
    else{
        hostNameLength = strlen(url);
    }

    // Hostname
    if (hostNameLength > 0 && hostNameLength <= URL_FIELD_MAX_LENGTH){
        memcpy(parameters->hostname, url, hostNameLength);

        // Special chars
        if (strrchr(parameters->hostname, '%')){
            if (decodePercent(parameters->hostname) == -1){
                return ERROR_DECODE;
            }
        }

        // Hostname or IP validity check
        if (ipAndHostChecker(parameters->hostname, parameters->ip) == -1){
            return ERROR_HOST_NOT_FOUND;
        }
    }
    else{
        fprintf(stderr,"ERROR: Invalid hostname size\n");
        return ERROR_FIELD_SIZE_EXCEEDED;
    }

    if (posColon && posColon < posSlash){
        // Case 1: ftp://hostname:port/ or ftp://hostname:port  

        // Port number
        posColon++;
        if (*posColon == portToken || (*posColon == '0' && *(posColon + 1) != portToken)){
            fprintf(stderr,"ERROR: Invalid port format\n");
            return ERROR_INVALID_URL_FORMAT;
        }

        char port[URL_MAX_PORT_LENGTH + 1];
        int portLenght = 0;

        char* posColonAux = posColon;
        while (*posColonAux != portToken){
            portLenght++;
            posColonAux++;
        }
    
        if (portLenght > URL_MAX_PORT_LENGTH){
            fprintf(stderr,"ERROR: Invalid port size\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        }

        memcpy(port, posColon, portLenght);
        port[portLenght] = '\0';

        if (strchr(port,'%')){
            if (decodePercent(port) == -1){
                return ERROR_DECODE;
            }
            portLenght = strlen(port);
        }
        
        for (int i = 0; i < portLenght; i++){
            if (!isdigit(port[i])){
                fprintf(stderr,"ERROR: Invalid port number\n");
                return ERROR_INVALID_URL_FORMAT;
            }
        }

        parameters->port = atoi(port);
        if (parameters->port <= 0 || parameters->port > FTP_MAX_PORT){
            fprintf(stderr,"ERROR: Invalid port number\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        } 
    }
    else if (!posColon || (posColon && posColon > posSlash)){
        // Case 2: ftp://hostname/ or ftp//hostname
        if (FTP_DEFAULT_PORT <= 0 || FTP_DEFAULT_PORT > FTP_MAX_PORT){
            fprintf(stderr,"ERROR: Invalid default port size\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        }
        parameters->port = FTP_DEFAULT_PORT;
    }
    

    // Url path
    if (posSlash){

        url = posSlash;
        char* lastToken = strdup(url) + 1;
        char* token = NULL;
        int i = 0;
        int totalCwd = 0;

        // Directory 
        while ((token = strchr(lastToken,'/'))){
            int cwdLen = token - lastToken;
            totalCwd++;

            if (cwdLen > 0 && cwdLen <= URL_FIELD_MAX_LENGTH && totalCwd <= URL_MAX_CWD) {
                char cwd[cwdLen + 1];
                memcpy(cwd, lastToken, cwdLen);
                cwd[cwdLen] = '\0';

                if (strchr(cwd, ';')){
                    fprintf(stderr,"ERROR: Invalid character in path\n");
                    return ERROR_INVALID_URL_FORMAT;
                }

                if (strchr(cwd, '%')){
                    if (decodePercent(cwd) == -1){
                        return ERROR_DECODE;
                    }
                    cwdLen = strlen(cwd);
                }

                memcpy(parameters->directories[i++], cwd, cwdLen);
            }
            else if (cwdLen == 0 && totalCwd <= URL_MAX_CWD){
                totalCwd++;
            }
            else{
                fprintf(stderr,"ERROR: Invalid directory size\n");
                return ERROR_FIELD_SIZE_EXCEEDED;
            }
            lastToken = token + 1;
        }
        
        parameters->directories[i][0] = '\0';


        // File name
        char* posV = strchr(lastToken, ';');
        int fileLen;
        if (posV){
            fileLen = posV - lastToken;
        }
        else{
            fileLen = strlen(lastToken);
        }

        if (fileLen > 0 && fileLen <= URL_FIELD_MAX_LENGTH){
            memcpy(parameters->filename, lastToken, fileLen);

            if (strchr(parameters->filename,'%')){
                if (decodePercent(parameters->filename) == -1){
                    return ERROR_DECODE;
                }
            }
        }
        else if (fileLen > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: Invalid filename size\n");
            return ERROR_FIELD_SIZE_EXCEEDED;
        }

        // Typecode
        if (posV){
            if (strncmp(posV + 1, "type=", 5) != 0){
                fprintf(stderr,"ERROR: Invalid typecode format\n");
                return ERROR_INVALID_URL_FORMAT;
            }
            char* typeCode = posV + 6;
            if (*typeCode == '\0') {
                fprintf(stderr, "ERROR: Typecode is missing\n");
                return ERROR_INVALID_URL_FORMAT;
            }

            int fakeTypeCodeLen = strlen(typeCode);
            char fakeTypeCode[fakeTypeCodeLen + 1];

            memcpy(fakeTypeCode, typeCode, fakeTypeCodeLen); 
            fakeTypeCode[fakeTypeCodeLen] = '\0';

            if (strchr(fakeTypeCode,'%')){
                if (decodePercent(fakeTypeCode) == -1) {
                    return ERROR_DECODE;
                }
            }
            
            fakeTypeCodeLen = strlen(fakeTypeCode);
        
            if (fakeTypeCodeLen == 1 && strchr("iIaA", fakeTypeCode[0])) {
                parameters->typecode = fakeTypeCode[0];
            } else {
                fprintf(stderr, "ERROR: Invalid typecode format\n");
                return ERROR_INVALID_URL_FORMAT;
            }
        }
        else {
            if (strlen(FTP_DEFAULT_TYPE_CODE) == 1 && strchr("iIaAdD", FTP_DEFAULT_TYPE_CODE[0])) {
                parameters->typecode = FTP_DEFAULT_TYPE_CODE[0];
            } else {
                fprintf(stderr, "ERROR: Invalid typecode format\n");
                return ERROR_INVALID_URL_FORMAT;
            }
        }
    }
    
    return 0;
}