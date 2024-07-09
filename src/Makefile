CC=gcc
RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lm -lncurses

TARGET=rpn
SRCS=rpn.c

OBJS=$(subst .c,.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)
	
*.o: *.c
	$(CC) $(CPPFLAGS) $(SRCS)

clean:
	$(RM) $(OBJS)

realclean:
	$(RM) $(OBJS) $(TARGET)
	
