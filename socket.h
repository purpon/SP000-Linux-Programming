#ifndef _SOCKET_H_
#define _SOCKET_H_

#define PORT 2000

struct socket_data {
	int sockfd;
	int client_sockfd;
   	int len;
};
typedef struct thread_arg Thread_arg;
struct thread_arg {
	int client_sockfd;
	int status;
	struct http_operations *ops;
};
#endif
