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
#include "socket.h"
#include "http.h"
#include <pthread.h>
#define CR '\r'
#define LF '\n'

#define MAX 4096
#define WORKER 4
#define MAX_THREADS 100
/* supporting functions */
static pid_t child_pid[4];

int http_listen(struct http_operations *ops,void* func)
{
    struct socket_data *priv= (struct socket_data *)ops->priv;
    int sockfd = priv->sockfd;
    int client_sockfd;
    struct sockaddr_in client_addr;
    pid_t self;
    int len = priv->len;
	self=getpid();
    /* Start Web server. */

	for(int child_number=0;child_number<WORKER;child_number++) {
		if(self==getpid() )
			child_pid[child_number] = fork();
	}
	if(self==getpid())
		return 0;
   /*Only the child process execuate*/ 
    printf("Child start listen\n");
	listen(sockfd, 30);
	pthread_t thread_pool[MAX_THREADS]={0};
	Thread_arg thread_arg[MAX_THREADS]={{0,0,0}};
	while(1) {
		unsigned int now_thread=MAX_THREADS-1;
		client_sockfd = accept(sockfd, &client_addr, &len);
		printf("client_sockfd=%d\n",client_sockfd);
		if (client_sockfd == -1) {
		perror("accept:");
		printf("Wrong return -1\n");
			return -1;
		}
		
		while(thread_arg[--now_thread].status)
			;
		thread_arg[now_thread].ops = ops;
		thread_arg[now_thread].status = 1;
		thread_arg[now_thread].client_sockfd = client_sockfd;
		pthread_create(&thread_pool[now_thread],NULL,(void *)func,&(*(thread_arg+now_thread)));
	}
}

/* callback functions */
int http_write(struct http_operations *ops, char *buf, int n)
{
    struct socket_data *priv = (struct socket_data *)ops->priv;
    int fd = priv->client_sockfd;

    write(fd, buf, n);
}

int http_read(struct http_operations *ops, char *netread,int client_sockfd)
{
    struct socket_data *priv = (struct socket_data *)ops->priv;
    int fd;

    int len;
    char readch;
    ssize_t n;

    fd = client_sockfd;

    len = 0;
    netread[0] = CR;
    netread[1] = LF;

    while (len < MAX) {
	n = read(fd, &readch, 1);
	if (n <= 0)
	    break;

	netread[len++] = readch;
	if (readch == LF)
	    break;
    }

    netread[len] = '\0';

    return len;
}

int http_open(struct http_operations *ops)
{
    int sockfd;
    int len;
    struct sockaddr_in server_addr;
    struct socket_data *priv = ops->priv;

    printf("in ops->open\n");
    /* 1. Create a socket. */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
	perror("socket:");
	return -1;
    }

    /* 2. Bind an address to the socket. */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, &server_addr, sizeof(struct sockaddr_in)) < 0) {
	perror("bind:");
	return -1;
    }
    len = sizeof(struct sockaddr_in);

    priv->sockfd = sockfd;
    priv->len = len;

    printf("socket:socket_listen\n");
    //socket_listen(ops);
}

void http_close(struct http_operations *ops)
{
    struct socket_data *priv = (struct socket_data *)ops->priv;
    int client_sockfd = priv->client_sockfd;
    int sockfd =  priv->sockfd;

    shutdown(client_sockfd, SHUT_RDWR);
    close(client_sockfd);
    close(sockfd);

#if __USE_FORK__
    /**
     * Terminate child process.
     */
    exit(1);
#endif
}

struct http_operations http_ops =
{
	open:	http_open,
	read:	http_read,
	write:	http_write,
	close:	http_close,
	listen: http_listen,
};

int main(int argc, char *argv[])
{
    http_ops.priv = (struct socket_data *)malloc(sizeof(struct socket_data));  
    http_register(&http_ops, SOCKET_OPS);
    printf("Parent wait\n");
	while (1) {
	sleep(0);
#if __USE_FORK__
//	int status;
//	wait(&status);
#endif
    }
}
