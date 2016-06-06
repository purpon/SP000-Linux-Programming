CC = gcc
OBJS = socket.o http.o
CFLAGS = -g -std=c99
LIBS = -lpthread

all: clean $(OBJS)
	$(CC) $(CFLAGS) -o dweb $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS)
