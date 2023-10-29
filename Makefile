DEBUG=-g -Wall -Wextra

all: bin/prog

bin:
	mkdir bin

bin/prog: main.c | bin
	gcc $(DEBUG) -I/usr/include -o $@ $< -luring

clean:
	rm -rf bin/prog
