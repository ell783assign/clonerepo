C_FILES= $(wildcard *.c)

OBJS = ${C_FILES:.c=.o}

TRACE_LEVEL := 0

ifneq (${TRACE_LEVEL}, 0)
C_FLAGS += -DBUILD_DEBUG
endif
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
	gcc -c $< ${C_FLAGS} -fPIC -g -O0 -o $@ -I.