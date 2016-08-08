/*		$Id: expand.c,v 1.43 2014/12/02 02:08:37 morris83 Exp $	*/

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

#define strsize 1024


int expand (char *orig, char *new, int newsize)
{
	int	o;	// orig str index
	int	n;	// new str index
	int	e;	//	envar index
	int	v;
	char* val; // environment variable value
	int	p; // pid index, (and many other indexes)
	char	pid[14];
	char	argchar[3];		// str version of the argcount
	char	argrqst[22];	// str of num of requested arg
	int	argrqstint;		// int argrequest
	char* argstr;	// str of the previously requested arg
	int	r;			// argrqst index
	DIR   *d;
  	struct dirent *dir;
  	char*	fname;	// file name of current file
  	char	context[5]; // context following *
  	int	c;			// context index
  	char* fend;	// end of file name 2b compared w/ context
  	int	f;			// file name index
  	char	*firstfile;
  	int	q;
  	char	exitstr[4];
  	int	tildelimit = 30;
  	char	tildeuser[tildelimit];	//name after tilde
  	int	t;		//tildeuser index
  	struct passwd *pw;
	const char *homedir;
	char	*recurseme;	//str to pass to processline
	int	fd[2];		//fd array for pipe
	char	outputbuf[2048];	//buffer str collected from pipe
	int	b;		//outputbuf index
	pid_t cpid;
	int	status;
	int	saveo;
	int	parens = 0;
	int	plr;
	
	
	d = opendir(".");
	
	snprintf(pid, 10,"%d",(int)getpid());
	snprintf(argchar, 3,"%d", (argcount-shift) );
	
	o	= 0;
	n	= 0;
	
	gotsigint = 0;
	
	while (gotsigint == 0)
	{
		if (orig[o] == '$')
		{
			o++;
			if (orig[o] == '{')
			{
				v = (o + 1);
				
				while (orig[o] != '}')
				{
					if (orig[o] == 0)
					{
						fprintf (stderr, "error: ");
						fprintf (stderr, "missing }\n");
						return -1;
					}
					o++;
				}
				
				orig[o] = 0;
				
				if (orig[v] == '}')
				{
					fprintf (stderr, "error: nothing between {}\n");
					return -1;
				}
				
				val = getenv( &(orig[v]) );
				
				if (val == NULL)
				{
					fprintf (stderr, "error: not an env var\n");
					return -1;
				}
				else
				{
					e = 0;
					while (val[e] != 0)
					{
						new[n] = val[e];
						e++; n++;
					}
				}
			}
			else if (orig[o] == '$')
			{
				p = 0;
				while (pid[p] != 0)
				{
					new[n] = pid[p];
					n++;
					p++;
				}

			}
			else if (orig[o] == '#')
			{
				p = 0;
				while (argchar[p] != 0)
				{
					new[n] = argchar[p];
					n++;
					p++;
				}

			}
			else if (isdigit(orig[o]) != 0)
			{	
				r = 0;
				while (isdigit(orig[o]) != 0)
				{
					argrqst[r] = orig[o];
					r++;
					o++;
				}
				o--;
				argrqst[r] = 0;
				
				argrqstint = (atoi(argrqst));
				
				if ( argrqstint < (argcount-shift) )
				{
					if (argcount == 1)
					{
						argstr = argarray[argrqstint+shift];
					}
					else
					{
						argstr = argarray[(argrqstint+1)+shift];
					}
					
					p = 0;
					while (argstr[p] != 0)
					{
						new[n] = argstr[p];
						n++;
						p++;
					}
				}
			}
			else if (orig[o] == '?')
			{
				snprintf(exitstr, 4,"%d", (exitint) );
				
				p = 0;
				while (exitstr[p] != 0)
				{
					new[n] = exitstr[p];
					n++;
					p++;
				}
			}
			else if (orig[o] == '(')
			{
				o++;
				parens++;
				recurseme = &orig[o];
				while (1)
				{
					if (orig[o] == '(')
					{
						parens++;
					}
					if (orig[o] == ')')
					{
						parens--;
						
						if (parens == 0)
						{
							orig[o] = 0;
							
							pipe(fd);
							
							cpid = fork();
							if (cpid < 0)
							{
								perror ("fork");
								return -1;
							}
							
							if (cpid == 0)
							{
								close(fd[0]);
								
								plr = processline(recurseme, 1, fd[1], NOWAIT);
								if (plr < 0)
								{
									fprintf(stderr, "processline error\n");
									return -1;
								}
								
								exit (0);
							}
							else
							{	
								close(fd[1]);
								
								while (read(fd[0], outputbuf, sizeof(outputbuf)) != 0)
								{	
									for (b=0; b<strlen(outputbuf); b++)
									{
										if (outputbuf[b] == '\n')
										{
											new[n] = ' ';
										}
										else
										{
											new[n] = outputbuf[b];
										}
										n++;
									}
									
									new[n] = 0;
									
									int	p;
									int	count = 1;
									for (p=0; p<strlen(new); p++)
									{
										if ((new[p] == ' ') || (new[p] == 0))
											count++;
									}
									
									if (count >= 19996)
									{
										fprintf(stderr, "can't pipe all: too long\n");
										break;
									}
									
									new[n] = ' ';
								}
							
								waitpid(cpid, &status, 0);
							}
						
							orig[o] = ')';
						
							break;
						}
					}
					else if (orig[o] == '\n')
					{
						orig[o] = ' ';
					}
					else if (orig[o] == 0)
					{
						fprintf (stderr, "error: invalid");
						fprintf (stderr, " syntax after $(\n");
						return -1;
					}
					o++;
				}
				
			}
			else
			{
				fprintf (stderr, "error: invalid syntax after $\n");
				return -1;
			}
			
		}
		else if (orig[o] == '*')
		{
			saveo = o;
			o++;
			if ( (orig[o] == ' ') || (orig[o] == 0) || (orig[o] == '"'))
			{
				if (d)
  				{
   				while ((dir = readdir(d)) != NULL)
    				{
      				fname = dir->d_name;
      				if (fname[0] != '.')
      				{
      					p = 0;
      					for (p=0; p<strlen(fname); p++)
      					{
      						new[n] = fname[p];
      						n++;
      					}
      					new[n] = ' ';
      					n++;
      				}
    				}

    				closedir(d);
  				}
			}
			else
			{
				c = 0;
				q = 0;
				while (1)
				{
					context[c] = orig[o];
					c++;
					o++;
					if (orig[o] == '/')
					{
						fprintf (stderr, "error: context includes '/'\n");
						return -1;
					}
					if (orig[o] == 0)break;
					if (orig[o] == ' ')break;
					if (orig[o] == '"')break;
				}
				context[c] = 0;
				
				if (d)
  				{
   				while ((dir = readdir(d)) != NULL)
    				{
      				fname = dir->d_name;
      				
      				if (q != 0)
      				{
		   				if ((strcmp(fname, firstfile)) == 0)
		   				{
		   					break;
		   				}
      				}
      				
      				if (q == 0)
      					firstfile = fname;
      				
      				q++;
      				
      				if (fname[0] != '.')
      				{
      					f = strlen(fname);
      					while (1)
      					{		
      						fend = &fname[f];
								
								if ((strcmp(context, fend)) == 0)
      						{
		   						p = 0;
									for (p=0; p<strlen(fname); p++)
									{
										new[n] = fname[p];
										n++;
									}
		   						new[n] = ' ';
		   						n++;
		   					}
		   					
		   					if (f == 0)
		   						break;
								
      						f--;
      					}
      				}
    				}

    				closedir(d);
  				}
			}
			
			o = saveo;			
		}
		else if (orig[o] == '\\')
		{
			o++;
			if (orig[o] == '*')
			{
				new[n] = '*';
				n++;
			}
			else
			{
				new[n] = '\\';
				n++;
			}
		}
		else if (orig[o] == '~')
		{
			o++;
			if ( (orig[o] == ' ') || (orig[o] == 0) )
			{
				pw = getpwuid(getuid());
  				homedir = pw->pw_dir;
			}
			else if ( (orig[o] >= 97) && (orig[o] <= 122) )
			{
				t = 0;
				while (1)
				{
					tildeuser[t] = orig[o];
					t++;
					o++;
					if (orig[o] == '/')break;
					if (orig[o] == ' ')break;
					if (orig[o] == 0)break;
					if (t > tildelimit)
					{
						fprintf(stderr, "error: username");
						fprintf(stderr, " too long\n");
						return -1;
					}
				}
				tildeuser[t] = 0;
				
				pw = getpwnam(tildeuser);
  				homedir = pw->pw_dir;
			}
			
			t = 0;
			while (homedir[t] != 0)
			{
				new[n] = homedir[t];
				t++;
				n++;
			}
		}
		else if (orig[o] == 0)
		{
			new[n] = 0;
			return 1;
		}
		else
		{
			new[n] = orig[o];
			n++;
		}
		
		o++;
		if (o >= 19998)
		{
			fprintf(stderr, "error: buffer overflow\n");
			return -1;
		}
		
	}
	
	if (gotsigint)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}
