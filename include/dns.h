#ifndef _DNS_H_
#define _DNS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char* reverse_dns_handler(const char* ip);
char* dns_handler(const char * hostname);

#endif // _DNS_H_