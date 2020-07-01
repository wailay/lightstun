CC = gcc
SOURCES = $(wildcard *.c)
OBJS = $(SOURCES:.c=.o)
TARGET = main

.PHONY : clean all

$(TARGET):$(OBJS)
	$(CC) $^ -o $@
 
%.o:%.c
	$(CC) -c $< 


clean:
	rm *.o $(TARGET)
	
all: clean $(TARGET)