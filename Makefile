#	$Id: Makefile,v 1.3 2014/12/02 04:33:28 morris83 Exp $

all:
	gcc -g -Wall -o msh msh.c arg_parse.c builtin.c expand.c stmts.c
	
clean:
	-rm *.o $(objects) msh
