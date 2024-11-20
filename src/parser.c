#include "parser.h"

#define FTP_DEFAULT_PORT 21
#define FTP_DEFAULT_TYPE_CODE 'i'
#define FTP_PREFIX_SIZE 6
#define FTP_MAX_PORT 65535
#define USER_ANONYMOUS "anonymous"
#define PASS_ANONYMOUS "up202108845@edu.fe.up.pt" // Email address (optional)


int decodePercent(char* parameter){
    
    int length = strlen(parameter);
    for (int i = 0; i < length; i++) {
        if (parameter[i] == '%') {
            if (i + 2 < length && isxdigit(parameter[i + 1]) && isxdigit(parameter[i + 2])) {
                char hex[3] = { parameter[i + 1], parameter[i + 2], '\0' }; 
                char eqChar = (char)strtol(hex, NULL, 16);
                parameter[i] = eqChar; 

                memmove(&parameter[i + 1], &parameter[i + 3], length - i - 2);
                length -= 2; 
            }
        }
    }
    return 0;
}

int ipAndHostChecker(char* hostname, char* ip){
    
    if (hostname == NULL){
        fprintf(stderr,"No hostname specified");
        return -1;
    }

    char* dnsResult = dnsLookup(hostname);
    if (dnsResult) {
        int dnsResultLength = strlen(dnsResult);
        strncpy(ip, dnsResult, dnsResultLength);
        ip[dnsResultLength] = '\0';
    } 
    else {
        if (isdigit(hostname[0])) {
            char* reverseDnsResult = reverseDnsLookup(hostname);
            if (reverseDnsResult) {
                int hostnameLength = strlen(hostname);
                strncpy(ip, hostname, hostnameLength);
                ip[hostnameLength] = '\0';
                int reverseDnsResultLength = strlen(reverseDnsResult);
                strncpy(hostname, reverseDnsResult, reverseDnsResultLength);
                hostname[reverseDnsResultLength] = '\0';
            } else {
                fprintf(stderr,"ERROR: Invalid host name or IP\n");
                return -1;
            }
        }
    }
    return 0;
}

int ftpUrlParser(const char* url, FTP_Parameters* parameters){

    if (url == NULL || parameters == NULL){
        fprintf(stderr,"ERROR: Null pointer\n");
        return -1;
    }

    // Url prefix
    if (strncmp(url,"ftp://",FTP_PREFIX_SIZE) != 0){
        fprintf(stderr,"ERROR: Invalid url prefix"); 
        return -1;
    }
    url += FTP_PREFIX_SIZE;

    // Username and password logic 
    char* posArr = strchr(url,'@');
    if (posArr != NULL){

        char* posColon = strchr(url, ':');
        if (posColon != NULL && posColon < posArr){     
            // Username and password  - Example: ftp://username:password@ftp.up.pt......
            // Username
            int usernameLength = posColon - url;
            if (usernameLength == 0){
                parameters->username[0] = '\0';
            }
            else if (usernameLength > 0 && usernameLength <= URL_FIELD_MAX_LENGTH){
                strncpy(parameters->username, url, usernameLength);
                parameters->username[usernameLength] = '\0';
                
                // Speacial chars
                if (strchr(parameters->username, '/')) {
                    fprintf(stderr, "ERROR: Username contains '/'\n");
                    return -1;
                }

                if (strrchr(parameters->username, '%')){
                    if (decodePercent(parameters->username) == -1){
                        return -1;
                    }
                }
            }
            else{
                fprintf(stderr,"ERROR: Username is too long\n");
                return -1;
            }

            // Password
            int passwordLength = posArr - posColon - 1;
            if (passwordLength == 0){
                parameters->password[0] = '\0';
            }
            else if (passwordLength > 0 && passwordLength <= URL_FIELD_MAX_LENGTH){
                strncpy(parameters->password,posColon + 1,passwordLength);
                parameters->password[passwordLength] = '\0';

                if (strchr(parameters->password, '/') || strchr(parameters->password, ':')) {
                    fprintf(stderr, "ERROR: Username contains ':' or '/'\n");
                    return -1;
                }

                if (strrchr(parameters->password, '%')){
                    if (decodePercent(parameters->password) == -1){
                        return -1;
                    }
                }
            }
            else{
                fprintf(stderr,"ERROR: Password is too long\n");
                return -1;
            }
        }
        else if (posColon == NULL || (posColon != NULL && posColon > posArr)){  
            // Only has username, but no password - Example: ftp://username@ftp.up.pt......
            // Username
            int usernameLength = posArr - url;
        
            if (usernameLength == 0){
                parameters->username[0] = '\0'; // Empty username
            }
            else if (usernameLength > 0 && usernameLength <= URL_FIELD_MAX_LENGTH) {
                strncpy(parameters->username, url, usernameLength);
                parameters->username[usernameLength] = '\0';

                if (strchr(parameters->username, '/')) {
                    fprintf(stderr, "ERROR: Username contains '/'\n");
                    return -1;
                }

                if (strrchr(parameters->username, '%')){
                    if (decodePercent(parameters->username) == -1){
                        return -1;
                    }
                }
            } else {
                fprintf(stderr, "ERROR: Username is too long\n");
                return -1;
            }
            // Password
            parameters->password[0] = '\0'; // Password not given, assume empty
        }
        url = posArr + 1;
    }
    else{
        // Anonymous account - Example: ftp://ftp.up.pt......
        int anonymousUserLength = sizeof(USER_ANONYMOUS);
        int anonymousPasswordLength = sizeof(PASS_ANONYMOUS);

        if (anonymousUserLength > URL_FIELD_MAX_LENGTH || anonymousPasswordLength > URL_FIELD_MAX_LENGTH){
            fprintf(stderr,"ERROR: Username or password is too long\n");
            return -1;
        }

        strncpy(parameters->username, USER_ANONYMOUS, anonymousUserLength);
        parameters->username[anonymousUserLength] = '\0';
        strncpy(parameters->password, PASS_ANONYMOUS, anonymousPasswordLength);
        parameters->password[anonymousPasswordLength] = '\0';
    }


    // Host and Port logic 
    char* posSlash = strchr(url, '/');
    if (posSlash != NULL){
        char* posColon = strchr(url,':');
        if (posColon != NULL && posColon < posSlash){
            // Hostname and port - Example: ftp://hostname:port/....
            // Host and port
            int hostNameLength = posColon - url;
            if (hostNameLength > URL_FIELD_MAX_LENGTH || hostNameLength <= 0){
                fprintf(stderr,"ERROR: Host name is too long\n");
                return -1;
            }

            strncpy(parameters->hostname, url, hostNameLength);
            parameters->hostname[hostNameLength] = '\0';

            // Decoding hostname
            if (strrchr(parameters->hostname, '%')){
                if (decodePercent(parameters->hostname) == -1){
                    return -1;
                }
            }

            // Hostname or IP validity check
            if (ipAndHostChecker(parameters->hostname, parameters->ip) == -1){
                return -1;
            }
            
            // Check if port is valid
            char* posColonAux = posColon;
            posColonAux++;
            if (posColonAux == posSlash){
                fprintf(stderr,"ERROR: Invalid port format\n");
                return -1;
            }

            while (posColonAux != posSlash){
                if (!isdigit(*posColonAux)){
                    fprintf(stderr,"ERROR: Invalid port format\n");
                    return -1;
                }
                posColonAux++;
            }
            
            // First digit can't be zero
            if (*(posColon + 1) == '0'){
                fprintf(stderr,"ERROR: Invalid port format\n");
                return -1;
            }

            parameters->port = atoi(posColon + 1);
            if (parameters->port <= 0 || parameters->port > FTP_MAX_PORT){
                fprintf(stderr,"ERROR: Invalid port number\n");
                return -1;
            } 
        }
        else if (posColon == NULL || (posColon != NULL && posColon > posSlash)){
            // Only has host, and default ftp port
            int hostNameLength = posSlash - url;
            if (hostNameLength > URL_FIELD_MAX_LENGTH || hostNameLength <= 0){
                fprintf(stderr,"ERROR: Host name is too long\n");
                return -1;
            }

            strncpy(parameters->hostname, url, hostNameLength);
            parameters->hostname[hostNameLength] = '\0';

            if (strrchr(parameters->hostname, '%')){
                if (decodePercent(parameters->hostname) == -1){
                    return -1;
                }
            }

             // Hostname or IP validity check
            if (ipAndHostChecker(parameters->hostname, parameters->ip) == -1){
                return -1;
            }

            parameters->port = FTP_DEFAULT_PORT;
        }
    }
    else{
        // No slash case
        char* posColon = strchr(url,':');
        if (posColon != NULL){

            // Host and port
            int hostNameLength = posColon - url;
            if (hostNameLength > URL_FIELD_MAX_LENGTH || hostNameLength <= 0){
                fprintf(stderr,"ERROR: Host name is too long\n");
                return -1;
            }

            strncpy(parameters->hostname, url, hostNameLength);
            parameters->hostname[hostNameLength] = '\0';

            if (strrchr(parameters->hostname, '%')){
                if (decodePercent(parameters->hostname) == -1){
                    return -1;
                }
            }

             // Hostname or IP validity check
            if (ipAndHostChecker(parameters->hostname, parameters->ip) == -1){
                return -1;
            }

            char* posColonAux = posColon;
            posColonAux++;
            if ((*posColonAux) == '\0'){
                fprintf(stderr,"ERROR: Invalid port format\n");
                return -1;
            }

            while ((*posColonAux) != '\0'){
                if (!isdigit(*posColonAux)){
                    fprintf(stderr,"ERROR: Invalid port format\n");
                    return -1;
                }
                posColonAux++;
            }
            
            // First digit can't be zero
            if (*(posColon + 1) == '0'){
                fprintf(stderr,"ERROR: Invalid port format\n");
                return -1;
            }

            parameters->port = atoi(posColon + 1);
            if (parameters->port <= 0 || parameters->port > FTP_MAX_PORT){
                fprintf(stderr,"ERROR: Invalid port number\n");
                return -1;
            }
        }
        else{
            // Only has host, and default ftp port
            int hostNameLength = strlen(url);
            if (hostNameLength > URL_FIELD_MAX_LENGTH || hostNameLength <= 0){
                fprintf(stderr,"ERROR: Host name is too long\n");
                return -1;
            }

            strncpy(parameters->hostname, url, hostNameLength);
            parameters->hostname[hostNameLength] = '\0';

            if (strrchr(parameters->hostname, '%')){
                if (decodePercent(parameters->hostname) == -1){
                    return -1;
                }
            }

             // Hostname or IP validity check
            if (ipAndHostChecker(parameters->ip, parameters->hostname) == -1){
                return -1;
            }

            parameters->port = FTP_DEFAULT_PORT;
        }
    }

    // Path format depends on the file system
    if (posSlash != NULL){
        url = posSlash;

        char* lastToken = strdup(url) + 1;
        char* token = NULL;
        int totalCwdLen = 0;

        // CWD
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
                }

                strncat(parameters->directories, cwd, cwdLen);
                strcat(parameters->directories, "/");
                totalCwdLen += cwdLen + 1;
    
            }
            lastToken = token + 1;
        }

        if (totalCwdLen > 0){
            parameters->directories[totalCwdLen - 1] = '\0';
        }
        else{
            parameters->directories[totalCwdLen] = '\0';
        }

        // Name and typecode
        char* posV = strchr(lastToken, ';');
        int fileLen;
        if (posV != NULL){
            fileLen = posV - lastToken;
        }
        else{
            fileLen = strlen(lastToken);
        }

        if (fileLen == 0){
            parameters->filename[0] = '\0';
        }
        else if (fileLen > 0 && fileLen <= URL_FIELD_MAX_LENGTH){
            strncpy(parameters->filename, lastToken, fileLen);
            parameters->filename[fileLen] = '\0';

            if (strchr(parameters->filename,'%')){
                if (decodePercent(parameters->filename) == -1){
                    return -1;
                }
            }
        }
        else {
            fprintf(stderr,"ERROR: File name is too long\n");
            return -1;
        }

        // Typecode
        if (posV != NULL){
            if (strncmp(posV + 1, "type=", 5) != 0){
                fprintf(stderr,"ERROR: Invalid typecode format\n");
                return -1;
            }
            char* typeCode = posV + 6;
            if ((*typeCode == 'i' || *typeCode == 'I' || *typeCode == 'a' || *typeCode == 'A' || *typeCode == 'd' || *typeCode == 'D') && *(typeCode + 1) == '\0'){
                parameters->typecode = *typeCode;
            }
            else{
                fprintf(stderr,"ERROR: Invalid typecode format\n");
                return -1;
            }
        }
        else {
            parameters->typecode = FTP_DEFAULT_TYPE_CODE;
        }
    }  
    else {
        parameters->directories[0] = '\0'; 
        parameters->filename[0] = '\0';
        parameters->typecode = '\0';
    }


    printf("File: %s\n", parameters->filename);
    printf("Directory: %s\n", parameters->directories);
    printf("Typecode: %c\n", parameters->typecode);
    printf("Username: %s\n", parameters->username);
    printf("Password: %s\n", parameters->password);
    printf("Domain: %s\n", parameters->hostname);
    printf("IP: %s\n", parameters->ip);
    printf("Port: %d\n", parameters->port);

    return 0;
}