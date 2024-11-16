#include "appliation_layer.h"

#define FTP_DEFAULT_PORT 21
#define FTP_PREFIX_SIZE 6
#define FTP_MAX_PORT 65535
#define h_addr h_addr_list[0]



// ftp://[<user>:<password>@][<host>:<port>/]<url-path>

char* reverse_dns_handler(const char* ip){
    struct in_addr addr;
    struct hostent *h;
 
    if (inet_aton(ip, &addr) == 0){
        perror("Invalid IP address");
        return NULL;
    }

    h = gethostbyaddr((const void*) &addr,sizeof(addr),AF_INET);
    return strdup(h->h_name);
}

char* dns_handler(const char * hostname){

    struct in_addr addr;
    struct hostent *h;

    if (inet_aton(hostname, &addr) != 0) {
        perror("Invaild hostname address");
        return NULL;
    }

     if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    return inet_ntoa(*((struct in_addr *) h->h_addr));
}


int parse_ftp_url(const char* url, FTP_Parameters* parameters){

    // Url prefix
    if (strncmp(url,"ftp://",FTP_PREFIX_SIZE) != 0){
        fprintf(stderr,"ERROR: Invalid url prefix"); 
        return -1;
    }

    printf("Url using file transfer protocol (FTP)\n");
    url += FTP_PREFIX_SIZE;

    // Username and password logic 
    char* posArr = strchr(url,'@');
    if (posArr != NULL){

        char* posColon = strchr(url, ':');
        if (posColon != NULL && posColon < posArr){     
            // Username and password 
            int usernameLength = posColon - url;
            if (usernameLength == 0){
                parameters->username[0] = '\0';
            }
            else if (usernameLength > 0 && usernameLength < URL_FIELD_MAX_LENGTH){
                strncpy(parameters->username, url, usernameLength);
                parameters->username[usernameLength] = '\0';

                 for (int i = 0; i < usernameLength; i++){
                    if (parameters->username[i] == '/'){
                        fprintf(stderr,"ERROR: Username contains '/'\n");
                        return -1;
                    }
                }
            }
            else{
                fprintf(stderr,"ERROR: Username is too long\n");
                return -1;
            }

            int passwordLength = posArr - posColon - 1;
            if (passwordLength == 0){
                parameters->password[0] = '\0';
            }
            else if (passwordLength > 0 && passwordLength < URL_FIELD_MAX_LENGTH){
                strncpy(parameters->password,posColon + 1,passwordLength);
                parameters->password[passwordLength] = '\0';

                for (int i = 0; i < passwordLength; i++){
                    if (parameters->password[i] == ':' || parameters->password[i] == '/'){
                        fprintf(stderr,"ERROR: Password contains ':' or '/'\n");
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
            if (url < posArr){       
                // Only has username, but no password 
                int usernameLength = posArr - url;
          
                if (usernameLength > 0 && usernameLength < URL_FIELD_MAX_LENGTH){
                    strncpy(parameters->username, url, usernameLength);
                    parameters->username[usernameLength] = '\0';

                        for (int i = 0; i < usernameLength; i++){
                        if (parameters->username[i] == '/'){
                            fprintf(stderr,"ERROR: Username contains '/'\n");
                            return -1;
                        }
                    }
                }
                else{
                    fprintf(stderr,"ERROR: Username is too long\n");
                    return -1;
                }

                parameters->password[0] = '\0';
            }
            else if (url == posArr){
                // Anonymous
               parameters->username[0] = '\0';
               parameters->password[0] = '\0';
            }
        }
        url = posArr + 1;
    }
    else{
        parameters->username[0] = '\0';
        parameters->password[0] = '\0';
    }


    // Host and Port logic 
    char* posSlash = strchr(url, '/');
    if (posSlash != NULL){
        char* posColon = strchr(url,':');
        if (posColon != NULL && posColon < posSlash){
            // Host and port
            int hostNameLength = posColon - url;
            if (hostNameLength > URL_FIELD_MAX_LENGTH || hostNameLength <= 0){
                fprintf(stderr,"ERROR: Host name is too long\n");
                return -1;
            }

            strncpy(parameters->hostname, url, hostNameLength);
            parameters->hostname[hostNameLength] = '\0';

            char* dnsResult = dns_handler(parameters->hostname);
            if (dnsResult != NULL) {
                strncpy(parameters->ip, dnsResult, URL_FIELD_MAX_LENGTH);
                parameters->ip[URL_FIELD_MAX_LENGTH] = '\0'; 
            } 
            else {
                char* reverseDnsResult = reverse_dns_handler(parameters->hostname);
                if (reverseDnsResult != NULL) {
                    strncpy(parameters->ip, parameters->hostname, URL_FIELD_MAX_LENGTH);
                    parameters->ip[URL_FIELD_MAX_LENGTH] = '\0';
                    strncpy(parameters->hostname, reverseDnsResult, URL_FIELD_MAX_LENGTH);
                    parameters->hostname[URL_FIELD_MAX_LENGTH] = '\0';
                } else {
                    fprintf(stderr,"ERROR: Invalid host name or IP\n");
                    return -1;
                }
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

            char* dnsResult = dns_handler(parameters->hostname);
            if (dnsResult != NULL) {
                strncpy(parameters->ip, dnsResult, URL_FIELD_MAX_LENGTH);
                parameters->ip[URL_FIELD_MAX_LENGTH] = '\0'; 
            } 
            else {
                char* reverseDnsResult = reverse_dns_handler(parameters->hostname);
                if (reverseDnsResult != NULL) {
                    strncpy(parameters->ip, parameters->hostname, URL_FIELD_MAX_LENGTH);
                    parameters->ip[URL_FIELD_MAX_LENGTH] = '\0';
                    strncpy(parameters->hostname, reverseDnsResult, URL_FIELD_MAX_LENGTH);
                    parameters->hostname[URL_FIELD_MAX_LENGTH] = '\0';
                } else {
                    fprintf(stderr,"ERROR: Invalid host name or IP\n");
                    return -1;
                }
            }

            parameters->port = FTP_DEFAULT_PORT;
        }
        url = posSlash + 1;
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

            char* dnsResult = dns_handler(parameters->hostname);
            if (dnsResult != NULL) {
                strncpy(parameters->ip, dnsResult, URL_FIELD_MAX_LENGTH);
                parameters->ip[URL_FIELD_MAX_LENGTH] = '\0'; 
            } 
            else {
                char* reverseDnsResult = reverse_dns_handler(parameters->hostname);
                if (reverseDnsResult != NULL) {
                    strncpy(parameters->ip, parameters->hostname, URL_FIELD_MAX_LENGTH);
                    parameters->ip[URL_FIELD_MAX_LENGTH] = '\0';
                    strncpy(parameters->hostname, reverseDnsResult, URL_FIELD_MAX_LENGTH);
                    parameters->hostname[URL_FIELD_MAX_LENGTH] = '\0';
                } else {
                    fprintf(stderr,"ERROR: Invalid host name or IP\n");
                    return -1;
                }
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
            if (hostNameLength > URL_FIELD_MAX_LENGTH){
                fprintf(stderr,"ERROR: Host name is too long\n");
                return -1;
            }

            strncpy(parameters->hostname, url, hostNameLength);
            parameters->hostname[hostNameLength] = '\0';

            char* dnsResult = dns_handler(parameters->hostname);
            if (dnsResult != NULL) {
                strncpy(parameters->ip, dnsResult, URL_FIELD_MAX_LENGTH);
                parameters->ip[URL_FIELD_MAX_LENGTH] = '\0'; 
            } 
            else {
                char* reverseDnsResult = reverse_dns_handler(parameters->hostname);
                if (reverseDnsResult != NULL) {
                    strncpy(parameters->ip, parameters->hostname, URL_FIELD_MAX_LENGTH);
                    parameters->ip[URL_FIELD_MAX_LENGTH] = '\0';
                    strncpy(parameters->hostname, reverseDnsResult, URL_FIELD_MAX_LENGTH);
                    parameters->hostname[URL_FIELD_MAX_LENGTH] = '\0';
                } else {
                    fprintf(stderr,"ERROR: Invalid host name or IP\n");
                    return -1;
                }
            }

            parameters->port = FTP_DEFAULT_PORT;
        }
    }

    // Path format depends on the file system
    if (posSlash != NULL){
        url = posSlash + 1;
        int urlPathLength = strlen(url);
        if (urlPathLength == 0){
            parameters->path[0] = '\0';
        }
        else if (urlPathLength > 0 && urlPathLength < URL_MAX_PATH_LENGTH){
            strncpy(parameters->path, url, urlPathLength);
            parameters->path[urlPathLength] = '\0';
        }
        else{
            fprintf(stderr,"ERROR: URL path is too long\n");
            return -1;
        }   
    }
    else{
        parameters->path[0] = '/';
    }


    printf("Path: %s\n", parameters->path);
    printf("Username: %s\n", parameters->username);
    printf("Password: %s\n", parameters->password);
    printf("Domain: %s\n", parameters->hostname);
    printf("IP: %s\n", parameters->ip);
    printf("Port: %d\n", parameters->port);

    return 0;

}

int main(int argc, char **argv){
    
    // Parse command line arguments
    if (argc != 2){
        fprintf(stderr,"ERROR: Incorrect command line arguments.\n");
        printf("Example: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    if (strcmp(argv[0], "./download") != 0){
        fprintf(stderr,"ERROR: Invalid command. Use 'download' as the first argument.\n");
        exit(-1);
    }

    // Parse other arguments
    FTP_Parameters ftpParams;
    memset(&ftpParams, 0, sizeof(FTP_Parameters));

    if (parse_ftp_url(argv[1], &ftpParams) != 0){
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