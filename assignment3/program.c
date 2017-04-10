#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/sem.h>
#include <semaphore.h>

#include <pthread.h>
#include <unistd.h>

#include <math.h>

#define NUM_COUNTERS 2
#define QUEUE_CAPACITY 2
#define LAMBDA  1
#define MU 		1
/****************************************************************************************/
/* Custom defines section.															    */
/****************************************************************************************/
#ifdef BUILD_DEBUG
#define TRACE(...) 	fprintf(stderr, "\e[1;32m\nTRACE  \t%10s\t%3d\e[0m\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "\e[1;33m\nWARN  \t%10s\t%3d\e[0m\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "\e[0;31m\nERROR  \t%10s\t%3d\e[0m\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ENTRY()		fprintf(stderr, "\nTRACE \t%10s\t%3d Enter {",__func__, __LINE__);
#define EXIT()		fprintf(stderr, "\nTRACE \t%10s\t%3d Exit }",__func__, __LINE__);
#else
#define TRACE(...) 	
#define WARN(...) 	
#define ERROR(...) 
#define ENTRY()
#define EXIT()
#endif

#define WAKEUP(...) fprintf(stderr, "\e[1;32m\nWAKEUP\t");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\e[0m");
#define SLEEP(...) fprintf(stderr, "\e[1;33m\nSLEEP\t");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\e[0m");
#define DROP(...) fprintf(stderr, "\e[0;31m\nDROP\t");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\e[0m");
#define CONSUME(...) fprintf(stderr, "\e[0;34m\nCONSUME\t");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\e[0m");
#define FINISH(...) fprintf(stderr, "\e[0;35m\nFINISHED\t");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\e[0m");
#define ARRIVAL(...) fprintf(stderr, "\e[0;36m\nARRIVAL\t");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\e[0m");

#define CONSOLE(...) 	fprintf(stderr,__VA_ARGS__)
#define TRUE  		(uint32_t)1
#define FALSE 		(uint32_t)0
/****************************************************************************************/

typedef struct circular_linked_list
{
	void *self;
	struct circular_linked_list *next;
	struct circular_linked_list *prev;
}CLL;

#define INIT_CLL_ROOT(LIST)					\
	(LIST).self = NULL;						\
	(LIST).prev = &(LIST);						\
	(LIST).next = &(LIST);

#define INIT_CLL_NODE(NODE,SELF)			\
	(NODE).self = (SELF);					\
	(NODE).next = NULL;					\
	(NODE).prev = NULL;		

//Read as Insert A after B
#define INSERT_AFTER(A,B)					\
	(A).next = (B).next;						\
	(A).next->prev = &(A);						\
	(B).next = &(A);							\
	(A).prev = &(B);	

//Read as Insert A before B
#define INSERT_BEFORE(A,B)					\
	(A).next = &(B);							\
	(B).prev->next = &(A);						\
	(A).prev = (B).prev;						\
	(B).prev = &(A);

#define REMOVE_FROM_LIST(A)					\
	(A).next->prev = (A).prev;					\
	(A).prev->next = (A).next;					\
	(A).prev = NULL;							\
	(A).next = NULL;			

#define NEXT_IN_LIST(NODE)						\
	(NODE).next->self

#define PREV_IN_LIST(NODE)						\
	(NODE).prev->self


typedef struct job
{
	CLL node;
	uint32_t pid;
	uint32_t arrival_time;
	uint32_t burst_time;

	int32_t owner_cpu;
}JOB;

/****************************************************************************************/
struct glob
{
	/* Global job queue */
	CLL queue;

	/* Number of counters */
	uint32_t num_counters;

	sem_t wakeup;

	/* Queue capacity */
	int32_t capacity;

	/* Currently filled */
	int32_t occupancy;

	int32_t jobs_in_service;

	/* Mutex to protect access to this job queue */
	pthread_mutex_t mutex;

	/* clock */
	uint32_t ticks;

	int32_t num_jobs;
}GLOBAL;

typedef struct thread_context
{
	pthread_t tid;
	int32_t thread_number;

	int32_t idle;

}THREAD_CONTEXT;


/*************************************************************************************/
void * consume(void *);
int32_t get_next_batch(CLL *);
static void get_next_job(int32_t *, uint32_t *, uint32_t *, float, float);
/*************************************************************************************/

static void get_next_job(int32_t *pid, uint32_t *arrival_time, uint32_t *burst_time, float lambda, float mu)
{
	static uint32_t prev_arrival_time = 0;
	static uint32_t next_pid = 0;

	long double random_val_1 = ((long double)rand()/ RAND_MAX);
	long double random_val_2 = ((long double)rand()/ RAND_MAX);

	/* Generate next arrival time*/
	*arrival_time = prev_arrival_time + (((-1.0/lambda)*(log(random_val_1)))*1000);
	prev_arrival_time = *arrival_time;

	/* Generate next burst time*/
	*burst_time = (((-1.0/mu)*(log(random_val_2)))*1000);

	*pid = ++next_pid;

	return;
}


int32_t get_next_batch(CLL *list)
{
	int32_t ret_val = 0;
	int32_t arrival_time;
	int32_t burst_time;

	long unsigned int length;
	static int32_t jobs_read=0;

	int32_t next_job_arrival_time=0;
	int32_t params_read;

	JOB *job=NULL;
	static JOB *save_for_next=NULL;

	char *job_line = NULL;

	/* Read next job */

	if(jobs_read < GLOBAL.num_jobs)
	{
		if(save_for_next != NULL)
		{
			job = save_for_next;
			save_for_next = NULL;
		}
		else
		{
			job = (JOB *)malloc(sizeof(JOB));
			if(job==NULL)
			{
				ERROR("Error allocating memory for job schedule");
				ret_val = -1;
				goto EXIT_LABEL;
			}
			INIT_CLL_NODE(job->node, job);
			job->owner_cpu = -1;
			get_next_job(&job->pid, &job->arrival_time, &job->burst_time, LAMBDA, MU);
		}
		INSERT_BEFORE(job->node, *list);
		jobs_read++;

		next_job_arrival_time = job->arrival_time;

		/* Check if others job also needs to be enqueued. Else, put it on hold. */
		while(jobs_read < GLOBAL.num_jobs)
		{
			job = (JOB *)malloc(sizeof(JOB));
			if(job==NULL)
			{
				ERROR("Error allocating memory for job schedule");
				ret_val = -1;
				goto EXIT_LABEL;
			}
			INIT_CLL_NODE(job->node, job);
			job->owner_cpu = -1;
			get_next_job(&job->pid, &job->arrival_time, &job->burst_time, LAMBDA, MU);

			if(next_job_arrival_time < job->arrival_time)
			{
				save_for_next = job;
				ret_val = job->arrival_time - next_job_arrival_time;
				break;
			}
			else if(next_job_arrival_time == job->arrival_time)
			{
				/* arrived at the same time. Process together */
				INSERT_BEFORE(job->node, *list);
				jobs_read++;
			}
			else
			{
				ERROR("Next job cannot arrive before!");
				exit(0);
			}
		}
		if(jobs_read == GLOBAL.num_jobs)
		{
			/* Do not call again */
			ret_val = 0;
		}
	}
	else
	{
		/* Nothing more to read */
		TRACE("Nothing more to read!");
	}

EXIT_LABEL:	
	return(ret_val);
}


static void print_help(char *argv[])
{
	fprintf(stdout, "%s [-s <num of counters>] [-c <queue capacity>] [-m <max customers>]\n", argv[0]);
	return;
}

int32_t main(int32_t argc, char *argv[])
{
	int32_t ret_val = 0;

	int32_t ii;

	THREAD_CONTEXT *consumers;

	CLL feed;
	JOB *job=NULL;

	GLOBAL.capacity = QUEUE_CAPACITY;
	GLOBAL.num_jobs = 10000;
	GLOBAL.num_counters = NUM_COUNTERS;

	while((ret_val = getopt(argc, argv, "hs:c:m:")) != -1)
	{
		switch(ret_val)
		{
		case 's':
			GLOBAL.num_counters = atoi(optarg);
			break;

		case 'c':
			GLOBAL.capacity = atoi(optarg);
			break;

		case 'm':
			GLOBAL.num_jobs = atoi(optarg);
			break;

		case 'h':
			print_help(argv);
			exit(0);
			break;

		default:
			ERROR("Improper inputs. Exiting");
			exit(0);
		}
	}

	if(optind < argc)
	{
		ERROR("Erroneous number of inputs.");
		exit(0);
	}

	fprintf(stderr, "Number of counters: %d\n", GLOBAL.num_counters);
	fprintf(stderr,"Line capacity: %d\n", GLOBAL.capacity);
	fprintf(stderr,"Number of customers: %d\n", GLOBAL.num_jobs);

	ret_val = 0;

	consumers = (THREAD_CONTEXT *)malloc(sizeof(THREAD_CONTEXT) * GLOBAL.num_counters);
	if(consumers == NULL)
	{
		ERROR("Error allocating memory for thread contexts.");
		ret_val = -1;
		goto EXIT_LABEL;
	}

	INIT_CLL_ROOT(GLOBAL.queue);
	ret_val = pthread_mutex_init(&GLOBAL.mutex, NULL);
	if(ret_val != 0)
	{
		ERROR("Error initializing mutex.");
		goto EXIT_LABEL;
	}

	GLOBAL.occupancy = 0;
	GLOBAL.jobs_in_service = 0;
	sem_init(&GLOBAL.wakeup, 0, 0);

	GLOBAL.ticks = 0;

	for(ii=0;ii<GLOBAL.num_counters;ii++)
	{
		consumers[ii].thread_number = ii;
		//sem_init(&consumers[ii].wakeup, 0, 0);
		consumers[ii].idle = TRUE;
		/* Start other threads */

		ret_val = pthread_create(&consumers[ii].tid, NULL, &consume, &consumers[ii].thread_number);
		if(ret_val!=0)
		{
			ERROR("Error starting consumer thread.");
			goto EXIT_LABEL;
		}
	}

	/* Read jobs from file into queue */
	INIT_CLL_ROOT(feed);
	do
	{
		ret_val = get_next_batch(&feed);
		if(ret_val==-1)
		{
			ERROR("Some error occured!");
			goto EXIT_LABEL;
		}
		else if(ret_val ==0)
		{
			TRACE("Nothing to do!");
		}
		else
		{
			/* Try to add as many jobs to the queue as possible */
			pthread_mutex_lock(&GLOBAL.mutex);

			for(job = (JOB *)NEXT_IN_LIST(feed);
				job!=NULL;
				job = (JOB *)NEXT_IN_LIST(feed))
			{
				REMOVE_FROM_LIST(job->node);
				ARRIVAL("[%d]\tTime: %u\tBurst: %u",job->pid, job->arrival_time, job->burst_time);
				if(GLOBAL.occupancy< GLOBAL.capacity)
				{
					if(GLOBAL.occupancy<NUM_COUNTERS)
					{
						TRACE("Wakeup counter.");
						sem_post(&GLOBAL.wakeup);
					}

					TRACE("[%d]Insert job %d. Occupancy=%d", 
													job->arrival_time, 
													job->pid, 
													GLOBAL.occupancy);
					INSERT_BEFORE(job->node, GLOBAL.queue);
					GLOBAL.occupancy += 1;
				}
				else
				{
					WARN("[%d]Dropping process %d", job->arrival_time, job->pid);
					DROP("[%d]", job->pid);
					free(job);
					job=NULL;
				}
			}
			pthread_mutex_unlock(&GLOBAL.mutex);		
			/* put this thread to sleep unitl it is time for next arrival */
			usleep(ret_val*1000);
		}
	}while(ret_val != 0);


EXIT_LABEL:
	if(consumers != NULL)
	{
		free(consumers);
	}
	return(ret_val);
}

void * consume(void *args)
{
	int32_t thread_number = *(int32_t *)args;
	JOB *job = NULL;
	int32_t block = TRUE;

	TRACE("Created thread %d", thread_number);

	while(1)
	{
		if(block==TRUE)
		{
			SLEEP("Counter %d going to sleep", thread_number);
			sem_wait(&GLOBAL.wakeup);
			WAKEUP("Counter %d woken up", thread_number);
		}
		/* Get lock on mutex to access job queue */
		pthread_mutex_lock(&GLOBAL.mutex);
		if(GLOBAL.occupancy>0)
		{
			for(job = (JOB *)NEXT_IN_LIST(GLOBAL.queue);
				job!=NULL;
				job = (JOB *)NEXT_IN_LIST(job->node))
			{
				if(job->owner_cpu==-1)
				{
					break;
				}
			}
			if(job==NULL && GLOBAL.jobs_in_service==0)
			{
				ERROR("This shouldn't have happened!");
				exit(0);
			}
			else if(job!=NULL)
			{
				TRACE("Counter %d processes job %d", thread_number, job->pid);
				CONSUME("[%d]", job->pid);
				GLOBAL.jobs_in_service +=1 ;
				job->owner_cpu = thread_number;
				pthread_mutex_unlock(&GLOBAL.mutex);
				usleep(job->burst_time*1000);
				pthread_mutex_lock(&GLOBAL.mutex);
				REMOVE_FROM_LIST(job->node);
				GLOBAL.occupancy -=1;
				GLOBAL.jobs_in_service -= 1;
				if(GLOBAL.occupancy>=NUM_COUNTERS)
				{
					block = FALSE;
				}
				else
				{
					block = TRUE;
				}
				TRACE("Counter %d finishes %d", thread_number, job->pid);
				FINISH("[%d]", job->pid);
				free(job);
				job = NULL;
			}
		}
		else if(GLOBAL.occupancy<NUM_COUNTERS)
		{
			block = TRUE;
		}
		pthread_mutex_unlock(&GLOBAL.mutex);
		/* Else wait on some job to arrive! */

	}

	return(NULL);
}
