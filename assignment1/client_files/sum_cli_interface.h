int processthiscmd(char *userinput)
{	int i,k,nomorewords,wordsfoundintext,fourthword;
	wordsfoundintext=0;
	
	//getting first word of user
	if(strlen(userinput)==0)
	{	//printf("\nNo input\n");
			return 0;
	}
	i=0;
	while((userinput[i]==' ')||(userinput[i]=='\t'))
	{	if((i==MAXCMDLENGTH-1)||(i==strlen(userinput)-1))
		{	i++;
			break;
		}
		i++;
	};
	if((i==MAXCMDLENGTH)||(i==strlen(userinput)))
	{	//printf("\nAll space input\n");
		return 0;
	}
	for(k=0;userinput[i]!=' ' && userinput[i]!='\t' && i<strlen(userinput);i++)
	{	cmdargs[0][k]=userinput[i];
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
		{	i++;
			k=0;
			//cmdargs[1][k]='"';
			//k++;
			if(userinput[i]=='"' || i==MAXCMDLENGTH || i==strlen(userinput) )
				nomorewords=1;
			else
			{	
				while( userinput[i]!='"' && i<strlen(userinput) && i<MAXCMDLENGTH)
				{		cmdargs[1][k]=userinput[i];i++;k++;
				};
				//cmdargs[1][k]='"';
				//k++;
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
	fourthword=0;
	i++;
	while(i<strlen(userinput)&&i<MAXCMDLENGTH-1)
	{	if(userinpuit[i]!=' ' && userinput[i]!='\t')
		{	fourthword=1;
			break;
		}	
		i++;
	}
	return wordsfoundintext+fourthword;
}
