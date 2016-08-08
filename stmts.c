/*		$Id: stmts.c,v 1.4 2014/12/05 03:44:43 morris83 Exp $	*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>

#include "proto.h"
#include "global.h"

#define LINELEN 200000




typedef struct
{
	int	kind; //0=if,1=while,2=else,3=end,4=other
	int	elsend;
	char	*cmd;
} line;

typedef struct
{
	int	nlines;
	int	alines;
	line	*lines;
} stmt_list;

line make_line(char *str, int type);
void print_list(stmt_list stl);
void exec_stmts(stmt_list sl);


int 	a;
int	f;
int	size;
int	openends;


line make_line(char *str, int type)
{
	char	command[LINELEN];
	int	c;
	int	len;
	
	
	line l;
	l.kind = type;
	
	if ((type <= 1) || (type == 4))
	{
		f = 0;
		while(str[f] == ' '){f++;}
	
		if (type == 0)
			f = (f + 2);
		else if (type == 1)
			f = (f + 5);
		
		while(str[f] == ' '){f++;}
		
		c = 0;
		while (str[f] != 0)
		{
			command[c] = str[f];
			c++;
			f++;
			if (c >= LINELEN)
			{
				dprintf(2, "error: command too long\n");
				break;
			}
		}
		command[c] = 0;
		
		len = strlen(command);
		if (command[len-1] == '\n')
				command[len-1] = 0;
		
		dprintf(1, "cmd='%s'\n", command);
		dprintf(1, "cmd='%c'\n", command[2]);
		
		l.cmd = command;
	}
	
	l.elsend = 0;
	
	return l;
}


void print_list(stmt_list stl)
{
	int	e;
	
	stl.nlines = a;
	
	dprintf(1, "stl:\n");
	
	for (e=0; e<=stl.nlines; e++)
	{
		dprintf(1, "line[%d]:kind'%d',", e, stl.lines[e].kind);
		dprintf(1, "elsend'%d',", stl.lines[e].elsend);
		dprintf(1, "cmd'%s'\n", stl.lines[e].cmd);
	}
}


void exec_stmts(stmt_list stl)
{
	int	e;
	int	f;
	
	
	e = 0;
	while (e <= stl.nlines)
	{
		if ( stl.lines[e].kind == 0)
		{
			if ( processline (stl.lines[e].cmd, 0, 1, WAIT | STMTS | EXP ) != 0 )
			{
				e = stl.lines[e].elsend;
				continue;
			}
			else
			{
				e++;
				processline (stl.lines[e].cmd, 0, 1, WAIT | STMTS | EXP );
			}
		}
		else if ( stl.lines[e].kind == 2)
		{
			f = e;
			e++;
			while ( e < stl.lines[f].elsend )
			{
				processline (stl.lines[e].cmd, 0, 1, WAIT | STMTS | EXP );
				e++;
			}
		}
	}
}


void stmts (char *firstline, int type, int linenum)
{
	stmt_list sl;
	int	stmtsize = 7;
   char	statement[stmtsize];
   char	buffer [LINELEN];
	int	s;
	int	i;
	int	openloc = linenum;
	
	
	a = 0;
	size = 16;
	openends = 1;
	
	sl.lines = (line *)malloc( size*(sizeof(line *)) );
	
	sl.lines[a] = make_line(firstline, type);
	
	dprintf(1, "line[%d]:kind'%d',", a, sl.lines[a].kind);
	dprintf(1, "elsend'%d',", sl.lines[a].elsend);
	dprintf(1, "cmd'%s'\n", sl.lines[a].cmd);
	a++;
	
	while (1)
	{
		fprintf (stderr, "> ");
		
		fgets (buffer, LINELEN, stdin);
		
		s = 0;
		i = 0;
		while (buffer[s] == ' '){s++;}
		while ((buffer[s] != ' ') && (buffer[s] != 0))
		{
			statement[i] = buffer[s];
			i++;
			if (i > 7){break;}
			s++;
		}
		if (statement[i-1] == '\n')
			statement[i-1] = 0;
		else
			statement[i] = 0;
		
		//dprintf(1, "stmt='%s'\n", statement);
		//dprintf(1, "buffer='%s'\n", buffer);
		
		if ( (strcmp(statement, "if")) == 0 )
		{
			sl.lines[a] = make_line(buffer, 0);
			stmts(buffer, 0, a);
		}
		else if ( (strcmp(statement, "while")) == 0 )
		{
			sl.lines[a] = make_line(buffer, 1);
			stmts(buffer, 0, a);
		}
		else if ( (strcmp(statement, "else")) == 0 )
		{
			sl.lines[a] = make_line(buffer, 2);
			sl.lines[openloc].elsend = a;
			openloc = a;
		}
		else if ( (strcmp(statement, "end")) == 0 )
		{
			sl.lines[a] = make_line(buffer, 3);
			sl.lines[openloc].elsend = a;
			dprintf(1, "line[%d]='%d',", a, sl.lines[a].kind);
			dprintf(1, "'%s'\n", sl.lines[a].cmd);
			openends--;
			if (openends == 0)
			{
				sl.nlines = a;
				print_list(sl);
				exec_stmts(sl);
				return;
			}
		}
		else
		{
			sl.lines[a] = make_line(buffer, 4);
		}
		
		print_list(sl);
		
		a++;
		
		if (a >= size)
		{
			sl.lines = realloc( sl.lines, (2*size)*(sizeof(line *)) );
			size = (2*size);
		}
		
	}
	
}
