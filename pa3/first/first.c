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
	unsigned long int tag;
	unsigned long int time;
	struct line *next;
} Line; //line is already a pointer

void freeCache(Line **cache, int n)
{
	int i;
	for(i = 0; i < n; i++)
	{
		Line *ptr = cache[i];
		while(ptr != NULL)
		{
			Line *hld = ptr;
			ptr = ptr->next;
			free(hld);
		}
		//free(cache[i]);
	}
	free(cache);
	return;
}
int checkFull(Line *head, int assoc) //true is 0, false is 1
{
	Line *ptr = head; int counter = 0;
	while(ptr != NULL)
	{
		counter++;
		ptr = ptr->next;
	}
	if(counter == assoc)
		return 0;
	return 1;
}
Line *LRUcheck(Line *head, int assoc)
{
	Line *ptr = head; int min = head->time; Line *hold = head;
	while(ptr != NULL)
	{
		if(ptr->time < min)
		{
			min = ptr->time;
			hold = ptr;
		}
		ptr = ptr->next;
	}
	return hold;
}	
void read(Line **cache, unsigned int setIndex, unsigned long int cacheTag, int numLines,char *policy) //an array of lines
{
	Line *ptr = cache[setIndex];
	while(ptr != NULL) //go through all the nodes unless there is a hit
	{
		if(ptr->tag == cacheTag)
		{
			hits++;
			ptr->time++;
			return;
		}
		ptr = ptr ->next;
	}	
	
	//miss
	miss++;
	reads++;
	int check = checkFull(cache[setIndex], numLines);

	//creates a node
	Line *tmp = (Line *) malloc(sizeof(Line));
	tmp->time = 0;
	tmp->tag = cacheTag;
	tmp->next = NULL;
	
	if(check == 0) //full amount of lines
	{
		if(strcmp(policy, "fifo\0") == 0)
		{
			Line *hold = cache[setIndex];
	       		cache[setIndex] = cache[setIndex]->next;
			free(hold);
		}
		else
		{
			Line *remove = LRUcheck(cache[setIndex], numLines);
			if(cache[setIndex] != NULL && cache[setIndex] == remove)
			{
				Line *hold = cache[setIndex];
				cache[setIndex] = cache[setIndex]->next;
				free(hold);
			}
			else
			{
				Line *ptr = cache[setIndex]; Line *prev;
				while(ptr != NULL && ptr != remove)
				{
					prev = ptr;
					ptr = ptr->next;
				}
				prev->next = ptr->next;
				free(ptr);
			}
		}
		
		//null check insert to the end
		if(cache[setIndex] == NULL)
			cache[setIndex] = tmp;
		else
		{	
			
			Line *ptr = cache[setIndex];
			while(ptr->next != NULL)
				ptr = ptr->next;
			ptr->next = tmp;
		
		}
	}
	else if(check == 1) //not full
	{
		//null check
		if(cache[setIndex] == NULL)
			cache[setIndex] = tmp;
		else
		{
		
			Line *ptr = cache[setIndex];
			while(ptr->next != NULL)
				ptr = ptr->next;
			ptr->next = tmp;
			
		}
	}

}
void write(Line **cache, unsigned int setIndex, unsigned long int cacheTag, int numLines, char *policy)
{
	Line *set = cache[setIndex];
	Line *ptr = set;
	while(ptr != NULL) //go through all the nodes unless there is a hit
	{
		if(ptr->tag == cacheTag) //hit
		{
			hits++;
			writes++;
			ptr->time++;
			return;
		}
		ptr = ptr ->next;
	}
	//miss
	writes++;
	read(cache, setIndex, cacheTag, numLines, policy);	
	return;
}


/* arg[1] cache size
 * arg[2] block size
 * arg[3] cache policy
 * arg[4] associativity (number of lines per set)
 * arg[5] trace file
*/

int main(int argc, char** argv) 
{
	if(argc != 6) //check for valid number of args
	{
		printf("not correct number of arguments :( \n");
		return -1;
	}
	//check the cache and block size
	int cacheSize = atoi(argv[1]);
	if((cacheSize == 0) || !((cacheSize & (cacheSize - 1)) == 0))
	{
		printf("sorry this doesn't work\n");
		return 0;
	}

	int blockSize = atoi(argv[2]);	
	if((blockSize == 0) || !((blockSize & (blockSize - 1)) == 0))
	{
		printf("sorry this doesn't work\n");
		return 0;
	}
	
	//policy
	char *policy = argv[3];

	//check the assoc
	char *assoc = argv[4];
	int numLines, numSets;
	if(strcmp(assoc, "direct") == 0) //one line per set
	{
		numLines = 1;
		numSets = cacheSize/(blockSize);
	}
	else if(strcmp(assoc, "assoc") == 0) //fully associative cache, one set many lines
	{
		numSets = 1;
		numLines = cacheSize/(blockSize * numSets);
	}
	else if(strstr(assoc, "assoc:") != NULL) //n lines per set
	{
		numLines = argv[4][6] - '0';
		if((numLines == 0) || !((numLines & (numLines - 1)) == 0))
			printf("not valid assoc\n");
		numSets = cacheSize/(blockSize * numLines);
	}
	else //invalid arg
	{
		printf("not a valid associativity :p\n");
		return 0;
	}

	//printf("cacheSize: %d\nblockSize: %d\nassoc: %s\n", cacheSize, blockSize, assoc);
	//printf("numSets: %d\nnumLines: %d\n", numSets, numLines);
	
	//calculate number of set, tag, and block bits
	int s = log(numSets)/log(2);
	int b = log(blockSize)/log(2);
	//int t = 48 - (b + s);
	//printf("tag bits: %d\nset bits: %d\nblock bits:%d\n", t, s, b);
	
	//create a hashtable-esque structure
	Line **cache = (Line **) malloc(sizeof(Line *) * numSets);
	int i;
	for(i = 0; i < numSets; i++)
	{
		cache[i] = NULL;
	}
	//trace file
	char *filename = argv[5];
	FILE *infile = fopen(filename, "r");
	
	char function = ' ';
	unsigned long int hexAddress = 0; //hex address parameter
	int mask = (1 << s) - 1;

	while(fscanf(infile, "%c %lx\n", &function, &hexAddress) != EOF && function != '#')
	{
		unsigned int setIndex = (hexAddress >> b) & mask;
		unsigned long int cacheTag = (hexAddress >>b) >> s;
		//printf("%c %lx %lu %u \n", function, hexAddress, cacheTag, setIndex);
		if(function == 'R')
			read(cache, setIndex, cacheTag, numLines, policy);
		else if(function == 'W')
			write(cache, setIndex, cacheTag, numLines, policy);
	}
	freeCache(cache, numSets);
	printf("Memory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", reads, writes, hits, miss);
	//free(tmp);
return 0;
}
