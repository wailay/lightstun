CC=gcc
SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
TARGET=lightstun
CFLAGS=-lssl -lcrypto
.PHONY : clean all debug

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
 
%.o:%.c
	$(CC) $(CFLAGS) -c $< 


debug: CFLAGS += -DDEBUG
debug: clean 
debug: $(TARGET)

clean:
	rm *.o $(TARGET)
	
all: clean $(TARGET)