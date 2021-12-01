main: main.c server.c
	gcc-11 -pthread main.c threadpool.c server.c -o main -ggdb