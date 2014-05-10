#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>

int
main ( int argc, char *argv[] )
{
	int socket_num;
	int* socket_add;
	unsigned socket_num2;
	socket_num = socket(AF_INET,SOCK_STREAM,0);
	fprintf(stderr,"%d\n",socket_num);
	close(socket_num);
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
