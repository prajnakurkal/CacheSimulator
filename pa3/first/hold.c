#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//global variables
int reads;
int writes;
int hits;
int miss;

//declare struct
typedef struct line
{
	int valid;
	char *tag;
	int time;
	struct line *next;
} Line; //line is already a pointer

Line **cache;

void freeCache(int n)
{
	int i;
	for(i = 0; i < n; i++)
	{
		cache[i] = NULL;
		free(cache[i]);
	}
	free(cache);
	return;
}
char* DecToBinary(unsigned long int dec)
{
	char *binary = (char *) malloc(sizeof(char) * 49);
	binary[48] = '\0';
	unsigned long int place = dec;
	unsigned long int index, twos;
	
	int i;
	for(i = 0; i < 48; i++)
		binary[i] = '0';
	while(place != 0)
	{
		index = log(place)/log(2);
		binary[47 - index] = '1';
		twos = pow(2, index);
		place -= twos;
