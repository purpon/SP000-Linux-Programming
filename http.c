/*
   web.c,
   a dirty embedded Web server.
*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "http.h"
#include "socket.h"
#include <pthread.h>
#define CR '\r'
#define LF '\n'

#define MAX 4096

static int http_get;
static struct http_operations *http_ops[MAX_OPS];

static int http_parser(char *s)
{
    if (s[0] == CR && s[1] == LF)
	return -1;
    else if (strncmp(s, "GET", 3) == 0)
	http_get = 1;
    return 0;
}

/*
 * Read pipe.
 */
void pipe_read(FILE *stream, struct http_operations *ops)
{
   char buffer[1024];

   while (!feof(stream) && !ferror(stream) &&
          fgets(buffer, sizeof(buffer), stream) != NULL)
	   ops->write(ops, buffer, strlen(buffer));
}

static void *http_main(int n)
{
	struct http_operations *ops;
	char buf[MAX];
	int ret;
	int fds[2];	
	FILE *stream;

	ops = (struct http_operations *)http_data[n].fops;

	/* blocking open */
	printf("ops->open\n");
	if (ops == NULL)
	   printf("ops = NULL\n");
	if (ops->open) {
	   ops->open(ops);
	} else {
	   printf("ops->open = NULL\n");
	}
	printf("exit ops->open\n");

	/*
	 * Create pipes. Close input pipe. Read output pipe.
 	 */
	pipe(fds);
	close(fds[0]);

        while (1) {
	   printf("ops->read\n");
	  // ret = ops->read(ops, buf);
	   printf("client: %s [%d]\n", buf, ret);
           http_parser(buf);
	   if (buf[0] == CR && buf[1] == LF)
	      break;
	}

	if (http_get) {
	   dup2(fds[1], 1);
           stream = fdopen(fds[1], "r");
           pipe_read(stream, ops);
	}

	printf("ops->close\n");
	ops->close(ops);

	return NULL;
}




void *http_handle_io(void * arg)
{
	char buf[MAX];
	int ret;
	int fds[2];	
	FILE *stream;
	int client_sockfd;
	int counts=2000;
	Thread_arg* T_arg= (Thread_arg*)arg;
	struct http_operations *ops;
	ops = (struct http_operations*)(T_arg->ops);
	client_sockfd = T_arg->client_sockfd;
	while (1) {
//	printf("ops->read\n");
	ret = ops->read(ops, buf,client_sockfd);
//	printf("client: %s [%d]\n", buf, ret);
		http_parser(buf);
	if (buf[0] == CR && buf[1] == LF)
		break;
	if (!counts--)
		return NULL;
	}
	if(http_get) {	
		write(client_sockfd,"HTTP/1.1 200 OK\n",16);
		write(client_sockfd,"Content-Type: text/html\n",24); 
		write(client_sockfd,"\n",1); 
		write(client_sockfd,"<hl>hihi<hl>",12); 
		write(client_sockfd,"\n\n",2); 
	}
	printf("ops->close\n");
	close(client_sockfd);
	T_arg->status = 0;
	return NULL;
}

/* API implementation */
int http_register(struct http_operations *ops, int opsno)
{
   	pthread_t thread_id1;

	switch (opsno) {		case SOCKET_OPS:
				http_data[SOCKET_OPS].fops = ops;
				//pthread_create(&thread_id1, NULL, &http_main, SOCKET_OPS);
				ops->open(ops);
				ops->listen(ops,&http_handle_io);
				break;
		case FILE_OPS:
			http_ops[FILE_OPS] = 
			       (struct http_operations *)ops;
			break;
		default:
			printf("error\n");
			return -1;
	}
	//printf("exit http_register\n");

	return 0;
}
