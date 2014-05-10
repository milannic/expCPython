#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

int
main ( int argc, char *argv[] )
{
	int socket_num;
	int* socket_add;
	unsigned socket_num2;
	fprintf(stderr,"%d\n",sizeof(socket_num));
	fprintf(stderr,"%d\n",sizeof(socket_add));
	fprintf(stderr,"%d\n",sizeof(socket_num2));
	socket_num = socket(AF_INET,SOCK_STREAM,0);
	fprintf(stderr,"%d\n",socket_num);
	shutdown(socket_num,0);
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
