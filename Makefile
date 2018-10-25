## Elliot and Liam's make file
all: 
	gcc -o server server.c -pthread
	gcc -o player player.c

