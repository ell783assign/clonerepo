#include <stdio.h>
#include <unistd.h>

int main()
{
	if(access("/home/ahm/program2b.c", F_OK)!=-1)
	printf("Exists");	
}
