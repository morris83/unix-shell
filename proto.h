/*		$Id: proto.h,v 1.11 2014/12/05 01:02:01 morris83 Exp $	*/

#pragma once

#ifndef PROTO_H_
#define PROTO_H_

int main (int mainargc, char **mainargv);
int processline (char *line, int infd, int outfd, int flags);

int arg_parse (char *line, char ***argvp);

int builtin (int	argnum, char **args, int infd, int outfd, int errfd);

int expand (char *orig, char *new, int newsize);

void stmts (char *firstline, int type, int linenum);

#endif
