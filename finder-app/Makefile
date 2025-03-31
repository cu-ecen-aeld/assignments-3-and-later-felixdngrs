APP = writer

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -fsanitize=address -O2 

$(APP): $(APP).o
	$(CC) -o $(APP) $(APP).o

$(APP).o: $(APP).c
	$(CC) -c $(APP).c

clean:
	rm $(APP) *.o

.PHONY: clean
