#ifndef SOCKETFUNCTIONS
#define SOCKETFUNCTIONS
#define MAX_APPLICATIONS 15
int createDomainServerSocket(char* domain_path);
int createDomainClientSocket(char* domain_path);
int createRawSocket();

#endif
