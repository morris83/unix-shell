/*		$Id: global.h,v 1.12 2014/12/03 04:52:56 morris83 Exp $	*/

#pragma once


#ifndef MAIN

#define EXTERN extern
#define INIT(x)

#else

#define EXTERN
#define INIT(x) = x 

#endif


#define WAIT		1
#define NOWAIT 	0
#define STMTS		2
#define NOSTMTS	0
#define EXP			4
#define NOEXP		0
#define OTHER		8
#define NOTHING	0


EXTERN int 		argcount;
EXTERN char** 	argarray;
EXTERN int 		shift;
EXTERN int		exitint;
EXTERN int		gotsigint;
EXTERN int		prompted;
