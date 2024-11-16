#include "dns.h"

#define h_addr h_addr_list[0]
#define IPV4   AF_INET

char* reverse_dns_handler(const char* ip){
    struct in_addr addr;
    struct hostent* h;
 
    if (inet_aton(ip, &addr) == 0){
        return NULL;
    }

    h = gethostbyaddr((const void*) &addr,sizeof(addr),IPV4);
    return strdup(h->h_name);
}

char* dns_handler(const char * hostname){

    struct in_addr addr;
    struct hostent* h;

    if (inet_aton(hostname, &addr) != 0) {
        return NULL;
    }

     if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    return inet_ntoa(*((struct in_addr *) h->h_addr));
}

