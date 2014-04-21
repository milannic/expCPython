#include<stdlib.h>
#include<stdio.h>

extern int my_socket(int domain, int type, int protocol);
extern int my_shutdown(int socket, int how);


#include	<stdlib.h>

	int
main ( int argc, char *argv[] )
{
	int ret = my_socket(0,0,0);
	int ret_2 = my_shutdown(ret,0);
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
