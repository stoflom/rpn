CC=gcc
RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lm -lncurses

TARGET=rpn
SRCS=rpn.c
HEADERS=

OBJS=$(subst .c,.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)
	
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

realclean:
	$(RM) $(OBJS) $(TARGET)
	
