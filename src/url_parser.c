#include "url_parser.h"

#define FTP_DEFAULT_PORT 21
#define FTP_DEFAULT_TYPE_CODE 'i'
#define FTP_PREFIX_SIZE 6
#define FTP_MAX_PORT 65535
#define USER_ANONYMOUS "anonymous"
#define PASS_ANONYMOUS "upXXXXXXXXX@edu.fe.up.pt" // Email address (optional)


int decodePercent(char* parameter){
    int length = strlen(parameter);
    for (int i = 0; i < length; i++) {
        if (parameter[i] == '%') {
            if (i + 2 < length && isxdigit(parameter[i + 1]) && isxdigit(parameter[i + 2])) {
                char hex[3] = { parameter[i + 1], parameter[i + 2], '\0' }; 
                char eqChar = (char)strtol(hex, NULL, 16);
                parameter[i] = eqChar; 

                if (memmove(&parameter[i + 1], &parameter[i + 3], length - i - 2) == NULL){
                    fprintf(stderr,"Memory allocation failed");
                    return -1;
                }
                length -= 2; 
            }
        }
    }
    return 0;
}

int ipAndHostChecker(char* hostname, char* ip){
    
    if (!hostname){
        fprintf(stderr,"No hostname specified");
        return -1;
    }

    char* dnsResult = dnsLookup(hostname);
    if (dnsResult) {
        // Ip = dnsResult
        int dnsResultLength = strlen(dnsResult);
        strncpy(ip, dnsResult, dnsResultLength);
    } 
    else {
        if (isdigit(hostname[0])) {
            char* reverseDnsResult = reverseDnsLookup(hostname);
            if (reverseDnsResult) {
                // Ip = hostname
                int hostnameLength = strlen(hostname);
                strncpy(ip, hostname, hostnameLength);

                // Hostname = reverseDnsResult
                int reverseDnsResultLength = strlen(reverseDnsResult);
                strncpy(hostname, reverseDnsResult, reverseDnsResultLength);
            } else {
                fprintf(stderr,"ERROR: Invalid host name or IP\n");
                return -1;
            }
        }
    }
    return 0;
}

int ftpUrlParser(const char* url, FTP_Parameters* parameters){

    if (!url || !parameters){
        fprintf(stderr,"ERROR: Null pointer\n");
        return -1;
    }

    // Url prefix
    if (strncmp(url,"ftp://",FTP_PREFIX_SIZE) != 0){
        fprintf(stderr,"ERROR: Invalid url prefix"); 
        return -1;
    }
    url += FTP_PREFIX_SIZE;

    // Username and password 
    char* posArr = strchr(url,'@');
    if (posArr){

        char* posColon = strchr(url, ':');
        int usernameLength;
        int passwordLength;
        if (posColon && posColon < posArr){    
            usernameLength = posColon - url;
            passwordLength = posArr - posColon - 1;
        }
        else if (!posColon || (posColon && posColon > posArr)){
            usernameLength = posArr - url;
            passwordLength = 0;
        }

        // Case 1: ftp://username:password@... or ftp://username@... 
        // Username
        if (usernameLength > 0 && usernameLength <= URL_FIELD_MAX_LENGTH){
            strncpy(parameters->username, url, usernameLength);
            
            // Prohibited chars
            if (strchr(parameters->username, '/')) {
                fprintf(stderr, "ERROR: Username contains '/'\n");
                return -1;
            }
            
            // Special chars
            if (strrchr(parameters->username, '%')){
                if (decodePercent(parameters->username) == -1){
                    return -1;
                }
            }
        }
        else if (usernameLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: Username is too long\n");
            return -1;
        }

        // Password
        if (passwordLength > 0 && passwordLength <= URL_FIELD_MAX_LENGTH){
            strncpy(parameters->password,posColon + 1,passwordLength);
            
            // Prohibited chars
            if (strchr(parameters->password, '/') || strchr(parameters->password, ':')) {
                fprintf(stderr, "ERROR: Username contains ':' or '/'\n");
                return -1;
            }

            // Special chars
            if (strrchr(parameters->password, '%')){
                if (decodePercent(parameters->password) == -1){
                    return -1;
                }
            }
        }
        else if (passwordLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: Password is too long\n");
            return -1;
        }

        url = posArr + 1;
    }
    else{
        // Case 2: ftp://ftp.up.pt......
        int anonymousUserLength = sizeof(USER_ANONYMOUS);
        int anonymousPasswordLength = sizeof(PASS_ANONYMOUS);

        if (anonymousUserLength > 0 && anonymousUserLength <= URL_FIELD_MAX_LENGTH && anonymousPasswordLength > 0 && anonymousPasswordLength <= URL_FIELD_MAX_LENGTH){
            strncpy(parameters->username, USER_ANONYMOUS, anonymousUserLength);
            strncpy(parameters->password, PASS_ANONYMOUS, anonymousPasswordLength);
        }
        else if (anonymousUserLength > URL_FIELD_MAX_LENGTH || anonymousPasswordLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr, "ERROR: Anonymous account username or password is too long\n");
            return -1;
        }
    }


    // Host and Port logic 
    char* posSlash = strchr(url, '/');
    char* posColon = strchr(url,':');
    int hostNameLength;
    char portToken;

    if ((posSlash && posColon && posColon < posSlash) || (!posSlash && posColon && posColon < posSlash)){
        hostNameLength = posColon - url; 
        if (posSlash){
            portToken = '/';
        }
        else{
            portToken = '\0';
        }
    }
    else if (posSlash && (!posColon || (posColon && posColon > posSlash))){
        hostNameLength = posSlash - url;
    }
    else{
        hostNameLength = strlen(url);
    }

    // Hostname
    if (hostNameLength > 0 && hostNameLength <= URL_FIELD_MAX_LENGTH){
        strncpy(parameters->hostname, url, hostNameLength);

        // Special chars
        if (strrchr(parameters->hostname, '%')){
            if (decodePercent(parameters->hostname) == -1){
                return -1;
            }
        }

        // Hostname or IP validity check
        if (ipAndHostChecker(parameters->hostname, parameters->ip) == -1){
            return -1;
        }
    }
    else{
        fprintf(stderr,"ERROR: Host name is too long\n");
        return -1;
    }

    if (posColon && posColon < posSlash){
        // Case 1: ftp://hostname:port/ or ftp://hostname:port  

        // Port number
        posColon++;
        if (*posColon == portToken || (*posColon == '0' && *(posColon + 1) != portToken)){
            fprintf(stderr,"ERROR: Invalid port format\n");
            return -1;
        }

        char port[URL_FIELD_MAX_LENGTH + 1];
        int actualPortLenght = 0;

        char* posColonAux = posColon;
        while (*posColonAux != portToken){
            actualPortLenght++;
            posColonAux++;
        }

        strncpy(port, posColon, actualPortLenght);
        port[actualPortLenght] = '\0';

        if (strchr(port,'%')){
            if (decodePercent(port) == -1){
                return -1;
            }
            actualPortLenght = strlen(port);
        }
        
        for (int i = 0; i < actualPortLenght; i++){
            if (!isdigit(port[i])){
                fprintf(stderr,"ERROR: Invalid port number\n");
                return -1;
            }
        }

        parameters->port = atoi(port);
        if (parameters->port <= 0 || parameters->port > FTP_MAX_PORT){
            fprintf(stderr,"ERROR: Invalid port number\n");
            return -1;
        } 
    }
    else if (!posColon || (posColon && posColon > posSlash)){
        // Case 2: ftp://hostname/ or ftp//hostname
        parameters->port = FTP_DEFAULT_PORT;
    }
    

    // Url path
    if (posSlash){

        url = posSlash;
        char* lastToken = strdup(url) + 1;
        char* token = NULL;
        int totalCwdLen = 0;

        // Directory 
        while ((token = strchr(lastToken,'/'))){
            int cwdLen = token - lastToken;
            if (cwdLen > 0){
                char cwd[cwdLen + 1];
                strncpy(cwd, lastToken, cwdLen);
                cwd[cwdLen] = '\0';

                if (strchr(cwd, ';')){
                    fprintf(stderr,"ERROR: Invalid character in path\n");
                    return -1;
                }

                if (strchr(cwd, '%')){
                    if (decodePercent(cwd) == -1){
                        return -1;
                    }
                    cwdLen = strlen(cwd);
                }

                strncat(parameters->directories, cwd, cwdLen);
                strcat(parameters->directories, "/");
                totalCwdLen += cwdLen + 1;
    
            }
            lastToken = token + 1;
        }

        if (totalCwdLen > 0 && totalCwdLen <= URL_MAX_PATH_LENGTH){
            parameters->directories[totalCwdLen - 1] = '\0';
        }
        else if (totalCwdLen > URL_MAX_PATH_LENGTH){
            fprintf(stderr,"ERROR: Path is too long\n");
            return -1;
        }

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
            strncpy(parameters->filename, lastToken, fileLen);

            if (strchr(parameters->filename,'%')){
                if (decodePercent(parameters->filename) == -1){
                    return -1;
                }
            }
        }
        else if (fileLen > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: File name is too long\n");
            return -1;
        }

        // Typecode
        if (posV){
            if (strncmp(posV + 1, "type=", 5) != 0){
                fprintf(stderr,"ERROR: Invalid typecode format\n");
                return -1;
            }
            char* typeCode = posV + 6;
            if (*typeCode == '\0') {
                fprintf(stderr, "ERROR: Typecode is missing\n");
                return -1;
            }

            int fakeTypeCodeLen = strlen(typeCode);
            char fakeTypeCode[fakeTypeCodeLen + 1];

            strncpy(fakeTypeCode, typeCode, fakeTypeCodeLen); 
            fakeTypeCode[fakeTypeCodeLen] = '\0';

            if (strchr(fakeTypeCode,'%')){
                if (decodePercent(fakeTypeCode) == -1) {
                    return -1;
                }
            }
            
            fakeTypeCodeLen = strlen(fakeTypeCode);
        
            if (fakeTypeCodeLen == 1 && strchr("iIaAdD", fakeTypeCode[0])) {
                parameters->typecode = fakeTypeCode[0];
            } else {
                fprintf(stderr, "ERROR: Invalid typecode format\n");
                return -1;
            }
        }
        else {
            parameters->typecode = FTP_DEFAULT_TYPE_CODE;
        }
    }
    
    return 0;
}