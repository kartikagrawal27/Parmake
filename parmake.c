/**
 * Parallel Make
 * CS 241 - Spring 2016
 */
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include "parser.h"
#include "queue.h"
#include "rule.h"

queue_t * ruleQueue;
queue_t * readyQueue;
queue_t * alreadyRan;
char ** targets;
char ** list_of_rules;
int number_of_rules;
int last_thread=1;

pthread_mutex_t ruleQueueMutex;
pthread_mutex_t readyQueueMutex;
pthread_mutex_t alreadyRanMutex;
pthread_mutex_t end_check;
sem_t oneReady;
sem_t flagger;

void parsed_new_target(char * target)
{
	rule_t * newRule = malloc(sizeof(rule_t));
	rule_init(newRule);
	newRule->target = malloc(sizeof(char)*(strlen(target)+1));
	strcpy(newRule->target,target);	
	queue_enqueue(ruleQueue,newRule);					
}

void parsed_new_dependency(char * target, char * dependency)
{
	rule_t  * checker;
	char * newdep = malloc(sizeof(char)*(strlen(dependency)+1));
	strcpy(newdep,dependency);
	for(int count = (ruleQueue->size);count>0;count--)
	{
		checker = (rule_t *)(queue_dequeue(ruleQueue));
		if(strcmp(checker->target,target)==0){
			queue_enqueue(checker->deps,newdep);
		}	
		queue_enqueue(ruleQueue,checker);
	}
}

void removeFiles(void * theRule)
{
	rule_t * currRule = ((rule_t *)theRule);
	int counter;
	int i=0;
	int FileFlag=1;
	char * ListTarget = list_of_rules[i];
	char * currTarget;
	for(counter=currRule->deps->size;counter>0;counter--)
	{
		currTarget = (char *)(queue_dequeue(currRule->deps));
		while(ListTarget!=NULL)
		{
			
			if(strcmp(currTarget,ListTarget)==0)
			{
				queue_enqueue(currRule->deps,currTarget);
				FileFlag=0;
				break;
				
			}
			
			i++;
			ListTarget=list_of_rules[i];
		}
		
		if(FileFlag==1)
		{
			free(currTarget);		
		}
		FileFlag=1;
		i=0;
		ListTarget = list_of_rules[i];
	}
}


void parsed_new_command(char * target, char * command)
{
	rule_t  * checker;
	char * newcom = malloc(sizeof(char)*(strlen(command)+1));
	strcpy(newcom, command);
	for(int count = (ruleQueue->size);count>0;count--)
	{
		checker = (rule_t *)(queue_dequeue(ruleQueue));
		if(strcmp(checker->target,target)==0)
		{
			queue_enqueue(checker->commands,newcom);
		}	
		queue_enqueue(ruleQueue,checker);
	}
		

}

int allfiles(void * theRule)
{
	int ready_to_go = 0;	
	int counter;
	rule_t * currRule = ((rule_t *)theRule);
	if(access(((rule_t *)theRule)->target, F_OK)==-1){					
		ready_to_go=1;
	}
	else
	{
		time_t depstime = 0;
		time_t ruletime = 0;
		struct stat * rule_file = malloc(sizeof(struct stat));
		struct stat * deps_file = malloc(sizeof(struct stat));
		if(stat(currRule->target,rule_file)==0)
		{
			ruletime = rule_file->st_mtime;
		}
		
		char * curDeps;
		for(counter = currRule->deps->size;counter>0;counter--)
		{
			curDeps=(char *)(queue_dequeue(currRule->deps));
			if(stat(curDeps,deps_file)==0)
			{
				depstime = deps_file->st_mtime;
			}		
			if(depstime > ruletime)
			{			
				ready_to_go = 1;
				free(curDeps);
				break;
			}
			free(curDeps);		
		}
		free(rule_file);
		free(deps_file);
		counter--;
	}						
	counter=ruleQueue->size;
	if(ready_to_go==1)
	{
		queue_destroy(currRule->deps);
		return 1;
	}	
	if(ready_to_go==0)
	{
		queue_destroy(currRule->deps);
		return 0;
	}	
	return 2;
}

void allrules(void * theRule)
{
	rule_t * currRule = ((rule_t *)theRule);
	int counter;
	int depsCounter;
	int satFlag=1;
	rule_t  * CurrTarget;
	char * ListTarget;
	for(depsCounter=currRule->deps->size; depsCounter>0;depsCounter--)
	{
		ListTarget = (char *)(queue_dequeue(currRule->deps));
		for(counter = alreadyRan->size;counter>0;counter--)
		{
			CurrTarget = (rule_t *)(queue_dequeue(alreadyRan));
			if(strcmp(CurrTarget->target,ListTarget)==0)
			{
				satFlag=0;
			}
			queue_enqueue(alreadyRan,CurrTarget);
		}
		counter = alreadyRan->size;			
		if(satFlag==0)
		{
			free(ListTarget);
		}	
		else
		{
			queue_enqueue(currRule->deps,ListTarget);
		}
		satFlag=1;	
	}
	
}
void iterator_checker()
{	
	int fileCount=0;
	int counter;
	int i=0;
	int mod_flag=2;
	int initial_empty=0;
	char * ListTarget = list_of_rules[i];
	char * currTarget;
	int total_rules;
	for(total_rules=ruleQueue->size;total_rules>0;total_rules--)
	{
		rule_t * currRule = (rule_t *)(queue_dequeue(ruleQueue));
		counter=currRule->deps->size;
		if(counter==0)
		{
			initial_empty=1;
		}
		if(initial_empty==0)
		{
			for(counter=currRule->deps->size;counter>0;counter--)
			{
				currTarget = (char *)(queue_dequeue(currRule->deps));
				while(i<number_of_rules)
				{
					if(strcmp(currTarget,ListTarget)==0)
					{
						fileCount++;
						break;
					}
					i++;
					ListTarget=list_of_rules[i];
				}
				queue_enqueue(currRule->deps,currTarget);
				i=0;
				ListTarget = list_of_rules[i];
			}	
			if((int)fileCount==(int)currRule->deps->size)
			{
				allrules(currRule);
			}
			else if(fileCount==0)
			{
				mod_flag=allfiles(currRule);
			}
			else
			{
				removeFiles(currRule);
				allrules(currRule);
			}
		}
		fileCount=0;
		initial_empty=0;
		if(currRule->deps->size==0)
		{
			if(mod_flag==0)
			{
				pthread_mutex_lock(&alreadyRanMutex);
				queue_enqueue(alreadyRan,currRule);
				pthread_mutex_unlock(&alreadyRanMutex);
			}
			else
			{
				pthread_mutex_lock(&readyQueueMutex);
				queue_enqueue(readyQueue,currRule);
				sem_post(&oneReady);
				pthread_mutex_unlock(&readyQueueMutex);			
			}
		}
		else
		{
			queue_enqueue(ruleQueue,currRule);	
		}
		mod_flag=2;
	}	
	
}

void readyRunner()
{
	
	int error=0;	
	char * currCommand;
	pthread_mutex_lock(&readyQueueMutex);
	if(readyQueue->size==0)
	{
		pthread_mutex_unlock(&readyQueueMutex);
		return;
	}
	while(readyQueue->size!=0)
	{
	    rule_t * currRule =(rule_t *)(queue_dequeue(readyQueue));		
		pthread_mutex_unlock(&readyQueueMutex);
		while(currRule->commands->size!=0)
		{
			currCommand=(char*)(queue_dequeue(currRule->commands));		
			error = system(currCommand);
			if(error != 0)
			{
				exit(1);
			}	
			free(currCommand);
		}
		pthread_mutex_lock(&readyQueueMutex);
		queue_enqueue(alreadyRan,currRule);					
		pthread_mutex_unlock(&readyQueueMutex);
	}
}

void entireRunner()
{
	pthread_detach(pthread_self());

	while((ruleQueue->size!=0 && (int)alreadyRan->size!=(int)number_of_rules) || last_thread)
	{
		pthread_mutex_lock(&ruleQueueMutex);
		iterator_checker();
		pthread_mutex_unlock(&ruleQueueMutex);
		
		sem_wait(&oneReady);
		readyRunner();
		if(ruleQueue->size==0 && readyQueue->size==0)
		{
			last_thread=0;
		}	
	}
	sem_post(&flagger);
}
int parmake(int argc, char **argv)
{
	int a;
	char * makefile=NULL;
	int numThreads=0;
	int remainder;
	int targets_size;
					
	int unspecifiedThreads=0;
	int i=0;
	while((a = getopt(argc, argv, "f:j:"))!=-1)
	{
		switch(a)
		{
			case 'f':				
				makefile=optarg;
				break;
			case 'j':
				numThreads=atoi(optarg);
				unspecifiedThreads=1;
				break;			
			default:
				continue;
		}
	}
	if(optind<argc)
	{
		int targets_size = optind-argc+1;		
		targets = malloc(sizeof(char *) * targets_size);
		
		for(remainder=optind; remainder<argc; remainder++)
		{
			targets[i] = malloc(strlen(argv[remainder]));
			strcpy(targets[i], argv[remainder]);
			i++;
		}
		targets[i+1]=NULL;
	}
	else
	{
		targets_size = 0;
		targets=malloc(sizeof(char *));
		targets[0]=NULL;
	}

	if(unspecifiedThreads==0)
	{
		numThreads=1;					
	}							

	if(makefile==NULL)
	{
		char * makefile1="makefile";
		char * Makefile="Makefile";
		if(access(makefile1,F_OK)==0)
		{
			makefile="makefile";			
		}
		else if(access(Makefile,F_OK)==0)
		{
			makefile="Makefile";			
		}
		else
		{
			return 1;
		}
	}
	ruleQueue = malloc(sizeof(queue_t));
	queue_init(ruleQueue);					
	readyQueue = malloc(sizeof(queue_t));
	queue_init(readyQueue);
	alreadyRan = malloc(sizeof(queue_t));
	queue_init(alreadyRan);
	
	parser_parse_makefile(makefile,targets,*parsed_new_target,*parsed_new_dependency,*parsed_new_command);		
	
	number_of_rules=ruleQueue->size;
	list_of_rules = malloc(sizeof(char *) * number_of_rules);
	int z;
	for(z=0;z<number_of_rules;z++)
	{
		rule_t * temporary = (rule_t *)(queue_dequeue(ruleQueue));
		list_of_rules[z] = malloc(sizeof(char) * (strlen(temporary->target)+1));
		strcpy(list_of_rules[z],temporary->target);
		queue_enqueue(ruleQueue,temporary);
	}
	
	pthread_t tid[numThreads];	
	sem_init(&oneReady,0,readyQueue->size);
	
	sem_init(&flagger,0,0);
	
	for(i=0;i<numThreads;i++)
	{
		pthread_create(&tid[i],NULL,(void *)*entireRunner,NULL);
	}

	sem_wait(&flagger);
	
	while(alreadyRan->size!=0)
	{
		rule_t * memStuff = (rule_t*)(queue_dequeue(alreadyRan));
		free(memStuff->target);
		rule_destroy(memStuff);
		free(memStuff);		
	}	
	free(alreadyRan);
	free(ruleQueue);
	free(readyQueue);
	for(z=0;z<number_of_rules;z++)
	{
		free(list_of_rules[z]);
	}
	free(list_of_rules);
	for(z=0;z<targets_size;z++)
	{
		free(targets);
	}
	return 0;
}