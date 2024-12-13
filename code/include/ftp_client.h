#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_

// Modules
#include "url_parser.h"
#include "ftp_socket.h"
#include "dns.h"

// Error codes
#define SUCCESS 0
#define ERROR_COMMAND_LINE_ARGS -1
#define ERROR_INVALID_URL -2
#define ERROR_LOGIN_FAILED -3
#define ERROR_DOWNLOAD_FAILED -4
#define ERROR_LOGOUT_FAILED -5

/**
 * @brief This function handles the logic of a download request to a ftp server.
 */
int main(int argc, char **argv);

#endif // _FTP_CLIENT_H_