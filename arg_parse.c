/*		$Id: arg_parse.c,v 1.15 2014/12/05 03:44:43 morris83 Exp $	*/

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

#include "proto.h"


int arg_parse (char *line, char ***argvp)
{
	int 	z;
	int	x;
   int	argnum;
	char 	**array;
	

	z = 0;
   argnum = 0;
   
   //count args
	while (line[z] != 0)
   {
   	while (line[z] == ' ') z++;
   	if (line[z] != 0)
   	{
   		argnum++;
   		while ((line[z] != ' ') && (line[z] != 0))
   		{
   			if (line[z] == '"')
   			{
   				z++;
   				while ((line[z] != '"') && (line[z] != 0))
   					{z++;}
   				if (line[z] == 0)
   				{
   					fprintf (stderr, "error: odd ");
   					fprintf (stderr, "number of quotes\n");
   					return -1;
   				}
   				else if (line[z] == '"')
   				{
   					z++;
   					continue;
   				}
   			}
   			else
   			{
   				if ((line[z] == 0) || (line[z] == ' '))
   				{
   					break;
   				}
   				z++;
   			}
   		}
   	}
   }
   
   if (argnum == 0){
   	return -1;
   }
   
   array = (char **)malloc( (argnum+1)*(sizeof(char **)) );
   
   if (array == NULL) {
   	fprintf(stderr, "malloc error\n");
   	return -1;
   }
   
   array[argnum] = NULL;
   
   z = 0; // pointer to char in original string
   x = 0; // pointer to char in new string
   argnum = 0;
   
   //set arg pointers in array & put EoS char after each arg
	while (line[z] != 0)
   {
   	if ((argnum >= 1) && (line[z] == ' '))
   	{
   		line[x] = 0;
   		x++; z++;
   	}
   	
   	while (line[z] == ' '){ z++;}
   	
   	if (line[z] != 0)
   	{
			
			line[x] = line[z];
			array[argnum] = &line[x];
			
			argnum++;
   		while ((line[z] != ' ') && (line[z] != 0))
   		{
   			if (line[z] == '"')
   			{
   				z++;
					line[x] = line[z];
   				while ((line[z] != '"') && (line[z] != 0))
   				{
   					z++; x++;
   					line[x] = line[z];
   				}
   			}
   			else
   			{
   				if ((line[z] == 0) || (line[z] == ' '))
   				{
   					break;
   				}
   				
   				while ((line[z] != ' ') && (line[z] != 0))
   				{
   					line[x] = line[z];
   					z++; x++;
   				}
   			}
   		}
   	}
   }
   line[x] = 0;
   
   *argvp = array;
   return argnum;
}
