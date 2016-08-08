/*		$Id: builtin.c,v 1.35 2014/12/05 01:35:27 morris83 Exp $	*/

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
#include <pwd.h>
#include <dirent.h>
#include <grp.h>
#include <time.h>

#include "proto.h"
#include "global.h"

#define	uid(x) x.st_uid
#define	gid(x) x.st_gid
#define	theperm(x) x.st_mode
#define	linknum(x) x.st_nlink
#define	bytsize(x) x.st_size
#define	modtime(x) x.st_mtime



void print_perms(mode_t perm, int outfd)
{
	dprintf(outfd, (perm & S_IRUSR) ? "r" : "-");
	dprintf(outfd, (perm & S_IWUSR) ? "w" : "-");
	dprintf(outfd, (perm & S_IXUSR) ? "x" : "-");
	dprintf(outfd, (perm & S_IRGRP) ? "r" : "-");
	dprintf(outfd, (perm & S_IWGRP) ? "w" : "-");
	dprintf(outfd, (perm & S_IXGRP) ? "x" : "-");
	dprintf(outfd, (perm & S_IROTH) ? "r" : "-");
	dprintf(outfd, (perm & S_IWOTH) ? "w" : "-");
	dprintf(outfd, (perm & S_IXOTH) ? "x " : "- ");
}


int builtin (int	argnum, char **args, int infd, int outfd, int errfd)
{
	int	i;
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	DIR   *d;
  	struct dirent *dir;
  	char*	fname;	// file name of current file
  	char	date[20]; // date str (mod time)
	char*	statfile;// name of file to be sstat-ed
	int	s;			// sstat arg index
	struct stat st;
	char	*firstfile; // name of 1st file in directory
	int	f;			// file index (for stopping inf loops)
	int	t;
	char	*type;
	char	readset[2048]; //read's env var val
	int	r;					//readset index
	
	
	//"aecho" is identical to the "echo" command: 
	//it simply repeats everything after echo to the screen
	if ((strcmp(args[0], "aecho")) == 0)
	{
		if ( (argnum >= 2) && ((strcmp(args[1], "-n")) == 0) )
		{
			for (i=2; i<argnum; i++)
			{
				dprintf(outfd, "%s", args[i]);
				if (i < (argnum-1))
				{
					dprintf(outfd, " ");
				}
			}
		}
		else
		{
			for (i=1; i<argnum; i++)
			{
				dprintf(outfd, "%s", args[i]);
				if (i < (argnum-1))
				{
					dprintf(outfd, " ");
				}
			}
			dprintf(outfd, "\n");
		}
		exitint = 0;
		
		return 1;
	}
	//"exit" exits the shell
	else if ((strcmp(args[0], "exit")) == 0)
	{
		if (args[1] != 0)
		{
			exitint = atoi(args[1]);
			
			exit(exitint);
		}
		else
		{
			exitint = 0;
			
			exit(0);
		}
		
		return 1;
	}
	//"envset" assigns a value to an environmental variable
	else if ( (strcmp(args[0], "envset")) == 0 )
	{
		if (argnum >= 3)
		{
			setenv(args[1], args[2], 1);
			exitint = 0;
		}
		else
		{
			dprintf(errfd, "error: not enough arguments\n");
			exitint = 1;
		}
		
		return 1;
	}
	//"envunset" makes an environmental variable have no value
	else if ( (strcmp(args[0], "envunset")) == 0 )
	{
		if (argnum >= 2)
		{
			if (getenv(args[1]) == NULL)
			{
				dprintf(errfd, "error: not an env var\n");
				exitint = 1;
			}
			else
			{
				unsetenv(args[1]);
				exitint = 0;
			}
		}
		else
		{
			dprintf(errfd, "error: not enough arguments\n");
			exitint = 1;
		}
		
		return 1;
	}
	//"cd" changes the current working directory of the shell
	else if ( (strcmp(args[0], "cd")) == 0 )
	{
		if (argnum >= 2)
		{
			if (chdir(args[1]) != 0)
			{
				dprintf(errfd, "error: not a valid path\n");
				exitint = 1;
			}
			else
			{
				exitint = 0;
			}
		}
		else
		{
			if (homedir == 0)
			{
				dprintf(errfd, "error: needs path or home dir\n");
				exitint = 1;
			}
			else
			{
				chdir(getenv("HOME"));
				exitint = 0;
			}
		}
		
		return 1;
	}
	//"shift" changes the value of each numbered argument
	//by the integer specified (makes their arg number smaller)
	else if ((strcmp(args[0], "shift")) == 0)
	{
		if (argnum < 2)
		{
			shift = 1;
			exitint = 0;
		}
		else
		{
			if ( atoi(args[1]) >= (argcount-shift) )
			{
				dprintf(errfd, "error: shift too big\n");
				exitint = 1;
			}
			else
			{
				shift = atoi(args[1]);
				exitint = 0;
			}
		}
		
		return 1;
	}
	//"unshift" undos the work done by shift (arg numbers bigger)
	else if ((strcmp(args[0], "unshift")) == 0)
	{
		if (argnum < 2)
		{
			shift = 0;
			exitint = 0;
		}
		else
		{
			if ( atoi(args[1]) > shift )
			{
				dprintf(errfd, "error: unshift too big\n");
				exitint = 1;
			}
			else
			{
				shift = shift - atoi(args[1]);
				exitint = 0;
			}
		}
		
		return 1;
	}
	//"sstat" is identical to "stat": it prints information
	//about the file or files specified to the screen
	else if ((strcmp(args[0], "sstat")) == 0)
	{
		if (argnum < 2)
		{
			dprintf(errfd, "error: sstat needs a file name\n");	
			exitint = 1;
   	}
   	else
   	{
			exitint = 0;
			
			s = 1;
			f = 0;
			
			while (s < argnum)
			{
				if (s > argnum) break;
				
				statfile = args[s];
				
				d = opendir(".");
				
				if (d)
  				{
   				while ((dir = readdir(d)) != NULL)
    				{
      				fname = dir->d_name;
      				
      				if (f == 0)
      					firstfile = fname;
      				
      				if (f != 0)
      				{
		   				if ((strcmp(fname, firstfile)) == 0)
		   				{
		   					dprintf(errfd, "error: unable to find '%s'\n", statfile);
		   					exitint = 1;
		   					closedir(d);
		   					s++;
		   					
		   					f = 0;
		   					break;
		   				}
      				}
      				
      				f++;

						if ((strcmp(statfile, fname)) == 0)
      				{
							dprintf(outfd, "%s ", fname);
							if( stat(fname, &st) != 0 )
							{
                        dprintf(errfd, "error: unable to stat that\n");
                        exitint = 1;
                        return 1;
                     }
                     else
							{
								struct passwd *filestruct = getpwuid(uid(st));
								if (filestruct->pw_name == NULL)
								{
									dprintf(outfd, "%d ", uid(st));
								}
								else
								{
									dprintf(outfd, "%s ", filestruct->pw_name);
								}
								
								struct group *grp = getgrgid(gid(st));
								if (grp->gr_name == NULL)
								{
									dprintf(outfd, "%d ", gid(st));
								}
								else
								{
									dprintf(outfd, "%s ", grp->gr_name);	
								}
								
								print_perms(theperm(st), outfd);
								
								t = strlen(fname);
								while (1)
								{
									if (t == 0)
										break;
									if (fname[t] == '.')
									{
										type = &fname[t+1];
										dprintf(outfd, "%s ", type);
									}
									t--;
								}
								
								dprintf(outfd, "%zu ", linknum(st));
								dprintf(outfd, "%zu ", bytsize(st));
								
								strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&modtime(st)));
								dprintf(outfd, "%s", date);
								dprintf(outfd, "\n");
								
								f = 0;
							}
							
							closedir(d);
							s++;
		   				
		   				break;
		   			}
      			}
  				}
			}
		}
		
		return 1;
	}
	//"read" is indentical to "envset" except that it
	//provides a whole line to assign to the variable
	else if ((strcmp(args[0], "read")) == 0)
	{
		if (argnum >= 2)
		{
			if (infd > 0)
			{
				FILE	*filefd = fdopen(infd, "r");
				fgets (readset, 2048, filefd);
			}
			else
			{
				fgets (readset, 2048, stdin);
			}
			while (1)
			{
				if (readset[r] == '\n')
					readset[r] = 0;
				
				if (readset[r] == 0)
					break;
				r++;
			}
			setenv(args[1], readset, 1);
			exitint = 0;
		}
		else
		{
			dprintf(errfd, "error: not enough arguments\n");
			exitint = 1;
		}
   	
   	return 1;
	}
	else
	{
		return 0;
	}
}
