#ifndef _DNS_H_
#define _DNS_H_

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Connection settings
#define h_addr h_addr_list[0]
#define IPV4   AF_INET

/**
 * @brief This function does a reverse DNS lookup. From a given IP address finds the corresponding hostname
 * @param ip The IP address
 * @return The hostname associated with the given IP address, or NULL if the lookup fails or the IP address is invalid.
 */
char* reverseDnsLookup(const char* ip);

/**
 * @brief This function does a DNS lookup. From a given hostname finds the corresponding IP address
 * @param hostname The hostname
 * @return The IP address associated with the given hostname, or NULL if the lookup fails or the hostname is invalid.
 */
char* dnsLookup(const char * hostname);

#endif // _DNS_H_