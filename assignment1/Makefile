C_SOURCES = $(wildcard *.c)

OBJS = ${C_SOURCES:*.c=*.o}

all: server

clean:
	rm -f server *.o *.d

start:
	./server >>logs/server.log 2>&1 &

stop:
	pkill server

server: ${OBJS}
	gcc ${OBJS} -o $@

%.o:%.c
	gcc -c $< -g -O0 -o $@