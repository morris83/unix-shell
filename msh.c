/*		$Id: msh.c,v 1.51 2014/12/05 02:12:40 morris83 Exp $	*/

/* CS 352 -- Mini Shell!  
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 */

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

#define MAIN
#include "proto.h"
#include "global.h"

#define LINELEN 200000


int processline (char *line, int infd, int outfd, int flags);


void sighandler (int signum)
{
	sigset_t x;
	sigemptyset (&x);
	sigaddset(&x, SIGINT);
	
	gotsigint = 1;
	
	dprintf(1, "\n");
	
	if ((argcount == 1) && (prompted == 0))
		fprintf (stderr, "%s ", getenv("P1"));
}


void killzombies (int s)
{
	while(waitpid(0, &s, WNOHANG) > 0){}
}


int main (int mainargc, char **mainargv)
{
   char	buffer [LINELEN];
   int	len;
   FILE	*fp;
   int	c;	//index for comment search
   int	s; //index for statement search
   int	stmtsize = 7;
   char	statement[stmtsize];
   int	i; // index for statement
   struct sigaction act;
   act.sa_handler = sighandler;
   act.sa_flags = SA_RESTART;
   
   sigaction(SIGINT, &act, NULL);
   
	
   gotsigint = 0;
   exitint = 0;
   shift = 0;
   setenv("P1", "%", 1);
   
   
   if (mainargc > 1)
   {
   	argcount = mainargc-1;
   	argarray = mainargv;
   	
   	fp = fopen (mainargv[1], "r");
   	
   	if (fp == NULL)
		{
		   perror ("error opening file.\n");
		   exitint = 127;
		   exit(127);
		}
   }
   else
   {
   	argcount = mainargc;
   	argarray = mainargv;
   	fp = stdin;
   }
	
   while (1)
   {
		prompted = 0;
		
		if (mainargc == 1)
			fprintf (stderr, "%s ", getenv("P1"));
		
		if (fgets (buffer, LINELEN, fp) != buffer)
		  	break;
		
		prompted	= 1;

		len = strlen(buffer);
		if (buffer[len-1] == '\n')
			 	buffer[len-1] = 0;
		
		
		//COMMENT REMOVAL:
		for (c=0; c<len; c++)
		{
			if (buffer[c] == '"')
			{
				c++;
				while (buffer[c] != '"')
				{
					if (buffer[c] == 0)
					{
						fprintf(stderr, "error: odd ");
						fprintf(stderr, "number of");
						fprintf(stderr, " quotes\n");
						main(argcount, argarray);
					}
					c++;
				}
			}
			else if (buffer[c] == '#')
			{
				if (c > 0)
				{
					if (buffer[c-1] != '$')
						buffer[c] = 0;
				}
				else
				{
					buffer[c] = 0;
				}
				break;
			}
		}
		
		
		//STATEMENTS:
		s = 0;
		i = 0;
		while (buffer[s] == ' '){s++;}
		if (buffer[s] == 0)
		{
			break;
		}
			
		while ((buffer[s] != ' ') && (buffer[s] != 0))
		{
			statement[i] = buffer[s];
			i++;
			if (i > 7){break;}
			s++;
		}
		statement[i] = 0;
		//dprintf(1, "stmt='%s'\n", statement);
		if ( (strcmp(statement, "if")) == 0 )
		{
			stmts(buffer, 0, 0); //0=if,1=while
			continue;
		}
		else if ( (strcmp(statement, "while")) == 0 )
		{
			stmts(buffer, 1, 0);
			continue;
		}
		
		
		if ( processline (buffer, 0, 1, WAIT | NOSTMTS | EXP ) < 0 ) 
		{
			dprintf(1, "main pl error\n");
		}
	}

   if (!feof(stdin))
      perror ("read");

   return 0;
}


int processline (char *line, int infd, int outfd, int flags)
{
	pid_t	cpid;
   int   status;
   int	argnum;
   char	**args;
   int	gotnew;
   char	new[LINELEN];
   int	newlen;
   int	cinfd	 =	infd;
   int	coutfd =	outfd;
   int	cerrfd =	2;
   int	p; //index for redirect/pipeline search
   int	namesize = 100;
   char	writeto[namesize];
   int	w; //index of writeto
   int	wappend; //append mode
   int	wstderr;
   char	readfrom[namesize];
   int	r; //index of readfrom
   char	pipeme [LINELEN];
   int	m;
   int	fd[2];
   int	pipecount = 0;
   int	prevfd;
	
	
	killzombies(status);
	
	for (m=0; m<2; m++)
		fd[m] = 0;
	
	
	//EXPANSION:
	if (flags & EXP)
	{
		gotnew = expand(line, new, LINELEN);
	
		if (gotnew == -1)
			return -1;
	}
	else
	{
		strcpy(new, line);
	}
	newlen = strlen(new);
	
	m = 0;
	if (flags & WAIT)
	{	
		while(1)
		{
			if (new[m] == '"')
			{
				m++;
				while (new[m] != '"'){m++;}
			}
			else if (new[m] == '|')
			{
				pipecount++;
			}
			else if (new[m] == 0)
			{
				break;
			}
			m++;
		}
	}
	
	
	//PIPELINES:
	if (pipecount > 0)
	{
		m = 0;
		for (p=0; p<newlen; p++)
		{
			if (new[p] == '"')
			{
				p++;
				while (new[p] != '"') {p++;}
			}	
			else if (new[p] == '|')
			{
				new[p] = 0;
				p++;
				while (new[p] == ' '){p++;}
				if (pipecount > 1)
				{
					while (new[p] != '|')
					{
						pipeme[m] = new[p];
						p++;
						m++;
					}
					p++;
				}
				else
				{
					while (new[p] != 0)
					{
						pipeme[m] = new[p];
						p++;
						m++;
					}
				}
				
				pipeme[m] = 0;
				
				pipe(fd);
				
				// A | - | -
				processline(new, cinfd, fd[1], NOWAIT | NOSTMTS | NOEXP );
				close(fd[1]);
				
				// - | B | -
				while (pipecount > 1)
				{
					prevfd = fd[0];
					pipe(fd);
					processline(pipeme, prevfd, fd[1], NOWAIT | NOSTMTS | NOEXP );
					close(prevfd);
					close(fd[1]);
					m = 0;
					while (new[p] == ' '){p++;}
					while ((new[p] != '|') && (new[p] != 0))
					{
						pipeme[m] = new[p];
						p++;
						m++;
					}
					p++;
					pipeme[m] = 0;
					pipecount--;
				}
				
				// - | - | C
				if (flags & WAIT) {
					processline(pipeme, fd[0], coutfd, WAIT | NOSTMTS | NOEXP );
				} else {
					processline(pipeme, fd[0], coutfd, NOWAIT | NOSTMTS | NOEXP );}
				
				close(fd[0]);
				
				killzombies(status);
				
				return 0;
			}
		}
	}
	
	
	//REDIRECTION:
	wappend = 0;
	wstderr = 0;
	for (p=0; p<newlen; p++)
	{
		if (new[p] == '"')
		{
			p++;
			while (new[p] != '"') {p++;}
		}	
		else if (new[p] == '<')
		{
			new[p] = ' ';
			p++;
			
			while (new[p] == ' ') {p++;}
			
			r = 0;
			while (r < namesize)
			{
				readfrom[r] = new[p];
				new[p] = ' ';
				p++;
				r++;
				if ((new[p] == 0) || (new[p] == '>') || (new[p] == ' ') || (new[p] == '<'))
				{
					readfrom[r] = 0;
					break;
				}
			}
			
			if (cinfd != infd) close(cinfd);
			
			cinfd = open(readfrom, O_RDONLY, S_IWUSR | S_IRUSR );
		}
		else if (new[p] == '>')
		{
			if (p > 1)
			{
				if ((new[p-1] == '2') && (new[p-2] == ' '))
				{
					new[p-1] = ' ';
					wstderr = 1;
				}
			}
			
			new[p] = ' ';
			p++;
			
			if (new[p] == '>')
			{
				wappend = 1;
				new[p] = ' ';
				p++;
			}
			
			while (new[p] == ' ') {p++;}
			
			w = 0;
			while (w < namesize)
			{
				writeto[w] = new[p];
				new[p] = ' ';
				p++;
				w++;
				if ((new[p] == 0) || (new[p] == '>') || (new[p] == ' ') || (new[p] == '<'))
				{
					writeto[w] = 0;
					break;
				}
			}
			
			if (coutfd != outfd) close(coutfd);
			
			if (wstderr == 1)
			{
				if (wappend == 1)
				{
					cerrfd = open(writeto, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR );
				}
				else
				{
					unlink(writeto);
					cerrfd = open(writeto, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR );
				}
			}
			else
			{
				if (wappend == 1)
				{
					coutfd = open(writeto, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR );
				}
				else
				{
					unlink(writeto);
					coutfd = open(writeto, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR );
				}
			}
		}
	}
	
	if (cinfd < 0)
	{
		dprintf(cerrfd, "error: file doesn't exist\n");
		return -1;
	}
	
	
	//ARGUMENT PROCESSING:
	argnum = arg_parse(new, &args);
   
   if (argnum <= 0)
   	return 0;
	
   
   //EXECUTION:
   if ((builtin(argnum, args, cinfd, coutfd, cerrfd)) == 1)
   {
   	free(args);
   	return 0;
   }

   cpid = fork();
   if (cpid < 0)
   {
   	perror ("fork");
   	return -1;
   }
    
   if (cpid == 0)
   {
   	if (gotsigint != 1)
   	{
			if (cinfd != 0)
			{
				dup2(cinfd, 0);
				close(cinfd);
			}
			if (coutfd != 1)
			{
				dup2(coutfd, 1);
				close(coutfd);
			}
			if (cerrfd != 2)
			{
				dup2(cerrfd, 2);
				close(cerrfd);
			}
			
			execvp(args[0], args);
			perror ("exec");

			exit (127);
      }
      else
      {
      	exit(1);
      }
   }
   
   killzombies(status);
   
   if (flags & WAIT)
	{
		if (waitpid(cpid, &status, 0) < 0)
		   perror ("wait");
		   free(args);
		exitint = WEXITSTATUS(status);
		return 0;
	}
	else
	{
		return cpid;
	}
}
