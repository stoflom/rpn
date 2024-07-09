CC=gcc
RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lm -lncurses

SRCS=rpn.c
OBJS=$(subst .c,.o,$(SRCS))

all: rpn

rpn: $(OBJS)
	$(CC) $(LDFLAGS) -o rpn $(OBJS) $(LDLIBS)
	
*.o: *.c
	$(CC) $(CPPFLAGS) $(SRCS)

clean:
	$(RM) $(OBJS)



