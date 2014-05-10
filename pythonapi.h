#ifndef PYTHON_CONCOORD_API
#define PYTHON_CONCOORD_API
int sc_socket(int domain,int type,int protocol);
int sc_connect(int socket, const struct sockaddr *address, socklen_t address_len);
ssize_t sc_send(int socket, const void *buffer, size_t length, int flags);
ssize_t sc_recv(int socket, const void *buffer, size_t length, int flags);
int sc_close(int fd);
void sc_pthread_exit(void* retval);
#endif
