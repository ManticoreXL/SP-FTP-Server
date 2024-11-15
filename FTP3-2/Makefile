all: cli.c srv.c
	gcc -o cli cli.c -g
	gcc -o srv srv.c -g

cli: cli.c
	gcc -o cli cli.c

srv: srv.c
	gcc -o srv srv.c