C_FILES= $(wildcard *.c)

OBJS = ${C_FILES:.c=.o}

########################################
all: scheduler

clean:
	rm -f *.o
	rm -f scheduler

start:
	./scheduler

stop:
	pkill scheduler

########################################

scheduler: ${OBJS}
	gcc $^ -o $@

%.o:%.c
	gcc -c $< -fPIC -g -O0 -o $@ -I.