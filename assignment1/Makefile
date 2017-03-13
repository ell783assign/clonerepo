SERVER_C_FILES = $(wildcard server_files/*.c)
CLIENT_C_FILES = $(wildcard client_files/*.c)
COMMON_C_FILES = $(wildcard utils/*.c)

SERVER_OBJS = ${SERVER_C_FILES:.c=.o}
CLIENT_OBJS = ${CLIENT_C_FILES:.c=.o}
COMMON_OBJS = ${COMMON_C_FILES:.c=.o}

BUILD_TRACE:=1

C_FLAGS := -O0
ifneq (${BUILD_TRACE},0)
	C_FLAGS+= -DBUILD_DEBUG
endif

.PHONY: common start_server start_client

########################################
all: common server client

clean:
	rm -f server_files/*.o client_files/*.o  server_files/*.d client_files/*.d
	rm -f utils/*.o utils/*.so utils/*.a
	rm -f server
	rm -f client

start_server:
	./server >logs/server.log 2>&1 &

start_client:
	./client	

stop:
	pkill server
	pkill client

########################################

common: ${COMMON_OBJS}
	gcc -shared ${C_FLAGS} -o utils/libutils.a $^ -Iutils/

server: ${SERVER_OBJS}
	gcc $^ ${C_FLAGS} -o $@ -Lutils/ -lutils

client: ${CLIENT_OBJS}
	gcc $^ ${C_FLAGS} -o $@ -Lutils/ -lutils

%.o:%.c
	gcc -c $< -fPIC -g ${C_FLAGS}  -o $@ -Iutils/
