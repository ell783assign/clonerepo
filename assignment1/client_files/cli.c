#include <utils.h>

#include <cli_interface.h>

char cmdargs[3][MAXCMDLENGTH];

void printdiagnosticmsg(char **msg)
{	
	printf("server says:%s\n",*msg);
}

int cmdnum(char *cmd)
{	
	if(strcmp("lls",cmd)==0)
		return 4;	
	if(strcmp("ls",cmd)==0)
			return 1;
	if(strcmp("cd",cmd)==0)
			return 2;
	if(strcmp("chmod",cmd)==0)
			return 3;
	if(strcmp("lcd",cmd)==0)
			return 5;
	if(strcmp("lchmod",cmd)==0)
			return 6;
	if(strcmp("put",cmd)==0)
			return 7;
	if(strcmp("get",cmd)==0)
			return 8;
	if(strcmp("close",cmd)==0)
			return 9;
	return -1;
}

int processthiscmd(char *userinput)
{	
	int wordsfoundintext = 0;

	int i,k,nomorewords;

	char *token;

	if(strlen(userinput)==0)
	{
		goto EXIT_LABEL;
	}
	/* Otherwise, we have a command */
	token = strtok(userinput, " ");
	if(!token)
	{
		ERROR("No Command found!\n");
	}
    
    strncpy(cmdargs[0], token, strlen(token));
	wordsfoundintext++;

    /* Try to find first parameter. It could open with a double quote */
    token = strtok(NULL, "\"");
    if(token!= NULL)
    {
    	/* Found, put the rest of things into argument 2. We can have no more args */
		strncpy(cmdargs[1], token, strlen(token));
		i=2;
		wordsfoundintext++;
		token = strtok(NULL, "\""); /* Skip the closing quote */
    }
    else
    {
    	i = 1;
    }

	while((token=strtok(NULL, " ")) != NULL)
	{
		strncpy(cmdargs[i], token, strlen(token));    			
		wordsfoundintext++;
	}

EXIT_LABEL:
	return (wordsfoundintext);	
}
#if 0
int processthiscmd(char *userinput)
{	
	int i,k,nomorewords,wordsfoundintext;
	wordsfoundintext=0;
	
	//getting first word of user
	if(strlen(userinput)==0)
	{	
		//printf("\nNo input\n");
		return 0;
	}
	i=0;
	while((userinput[i]==' ')||(userinput[i]=='\t'))
	{	
		if((i==MAXCMDLENGTH-1)||(i==strlen(userinput)-1))
		{	
			i++;
			break;
		}
		i++;
	};
	if((i==MAXCMDLENGTH)||(i==strlen(userinput)))
	{	
		//printf("\nAll space input\n");
		return 0;
	}
	for(k=0;userinput[i]!=' ' && userinput[i]!='\t' && i<strlen(userinput);i++)
	{	
		cmdargs[0][k]=userinput[i];
		k++;			
	}
	while((userinput[i]==' '||userinput[i]=='\t')&& i<MAXCMDLENGTH && i<strlen(userinput))
	{	i++;
	};
	cmdargs[0][k]='\0';
	//printf("\nWe will process first word as %s",cmdargs[0]);
	wordsfoundintext++;
	//getting second word considering double quotes if needed
	nomorewords=0;
	if(i<strlen(userinput)&&i<MAXCMDLENGTH-1)
	{
		if(userinput[i]=='"')
		{	
			i++;
			k=0;
			cmdargs[1][k]='"';
			k++;
			if(userinput[i]=='"' || i==MAXCMDLENGTH || i==strlen(userinput) )
				nomorewords=1;
			else
			{	
				while( userinput[i]!='"' && i<strlen(userinput) && i<MAXCMDLENGTH)
				{		cmdargs[1][k]=userinput[i];i++;k++;
				};
				cmdargs[1][k]='"';
				k++;
				if(i==MAXCMDLENGTH || i==strlen(userinput))
					nomorewords=1;
				else
					i++;
			}
		}
		else
		{	k=0;
			while(i<strlen(userinput) && i<MAXCMDLENGTH && userinput[i]!=' ' && userinput[i]!='\t')
			{	cmdargs[1][k]=userinput[i];i++;k++;
			}
		}
	}
	else
		nomorewords=1;
	if(nomorewords!=1)
	{	cmdargs[1][k]='\0';
		//printf("\nWe will process second word as %s",cmdargs[1]);
		wordsfoundintext++;
	}
	while((userinput[i]==' '||userinput[i]=='\t') && i<MAXCMDLENGTH && i<strlen(userinput))
	{	i++;
	};
	//getting third word
	nomorewords=0;
	if(i<strlen(userinput)&&i<MAXCMDLENGTH-1)
	{
		if(userinput[i]=='"')
		{	i++;
			k=0;
			cmdargs[2][k]='"';
			k++;
			if(userinput[i]=='"' || i==MAXCMDLENGTH || i==strlen(userinput) )
				nomorewords=1;
			else
			{	
				while( userinput[i]!='"' && i<strlen(userinput) && i<MAXCMDLENGTH)
				{		cmdargs[2][k]=userinput[i];i++;k++;
				};
				cmdargs[2][k]='"';
				k++;
				if(i==MAXCMDLENGTH || i==strlen(userinput))
					nomorewords=1;
			}
		}
		else
		{	k=0;
			while(i<strlen(userinput) && i<MAXCMDLENGTH && userinput[i]!=' ' && userinput[i]!='\t')
			{	cmdargs[2][k]=userinput[i];i++;k++;
			}
		}
	}
	else
		nomorewords=1;
	if(nomorewords!=1)
	{	cmdargs[2][k]='\0';
		//printf("\nWe will process third word as %s\n",cmdargs[2]);
		wordsfoundintext++;
	}
	return wordsfoundintext;
}

#endif