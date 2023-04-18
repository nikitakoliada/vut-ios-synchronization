CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pthread -Werror -pedantic
TARGET=proj2
SRC=proj2.c
HEADER=header.h

#Author - Nikita Koliada
#LOGIN - xkolia00

$(TARGET): $(SRC) $(HEADER)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)