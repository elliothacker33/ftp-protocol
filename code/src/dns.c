#include "dns.h"

char* reverseDnsLookup(const char* ip){
    if (!ip){
        return NULL;
    }
    struct in_addr addr;
    struct hostent* h;

    if (inet_aton(ip, &addr) == 0){
        return NULL;
    }

    h = gethostbyaddr((const void*) &addr,sizeof(addr),IPV4);
    if (h == NULL){
        herror("gethostbyaddr()");
        return NULL;
    }
    return strdup(h->h_name);
}

char* dnsLookup(const char* hostname){
    if (!hostname){
        return NULL;
    }
    struct in_addr addr;
    struct hostent* h;

    if (inet_aton(hostname, &addr) != 0) {
        return NULL;
    }

     if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        return NULL;
    }
    return inet_ntoa(*((struct in_addr *) h->h_addr));
}

