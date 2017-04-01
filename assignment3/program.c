#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/sem.h>
#include <semaphore.h>

#include <pthread.h>
#include <unistd.h>

#define NUM_COUNTERS 2
#define QUEUE_CAPACITY 2
/****************************************************************************************/
/* Custom defines section.															    */
/****************************************************************************************/
#ifdef BUILD_DEBUG
#define TRACE(...) 	fprintf(stderr, "\nTRACE  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define WARN(...) 	fprintf(stderr, "\nWARN  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ERROR(...)  fprintf(stderr, "\nERROR  \t%10s\t%3d\t", __func__, __LINE__);fprintf(stderr, __VA_ARGS__)
#define ENTRY()		fprintf(stderr, "\nTRACE \t%10s\t%3d Enter {",__func__, __LINE__);
#define EXIT()		fprintf(stderr, "\nTRACE \t%10s\t%3d Exit }",__func__, __LINE__);
#else
#define TRACE(...) 	
#define WARN(...) 	
#define ERROR(...) 
#define ENTRY()
#define EXIT()
#endif

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

	/* Counting semaphore */
	sem_t queue_lock;

	/* Queue capacity */
	int32_t capacity;

	/* Currently filled */
	int32_t occupancy;

	int32_t jobs_in_service;

	/* Mutex to protect access to this job queue */
	pthread_mutex_t mutex;

	/* clock */
	uint32_t ticks;

 	/* Job file pointer */
	FILE *job_file;

	int32_t num_jobs;

}GLOBAL;

typedef struct thread_context
{
	pthread_t tid;
	int32_t thread_number;

}THREAD_CONTEXT;


/*************************************************************************************/
void * consume(void *);
int32_t get_next_batch(CLL *);
/*************************************************************************************/



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
	if(GLOBAL.num_jobs == -1)
	{
		if(getline(&job_line, &length, GLOBAL.job_file) <-1)
		{
			ERROR("Error reading line from file.");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		if((params_read=sscanf(job_line, "%d", &GLOBAL.num_jobs))<1)
		{
			ERROR("Malformed input file.");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		TRACE("Number of jobs: %d", GLOBAL.num_jobs);
	}
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
			if(getline(&job_line, &length, GLOBAL.job_file) == -1)
			{
				ERROR("Error reading line from file.");
				ret_val = -1;
				goto EXIT_LABEL;
			}

			job = (JOB *)malloc(sizeof(JOB));
			if(job==NULL)
			{
				ERROR("Error allocating memory for job schedule");
				ret_val = -1;
				goto EXIT_LABEL;
			}
			INIT_CLL_NODE(job->node, job);
			job->owner_cpu = -1;
			if((params_read = sscanf(job_line, "%d %d %d", &job->pid,
															&job->arrival_time, 
														  	&job->burst_time)) < 3)
			{
				ERROR("Malformed input line. %d", params_read);
				ret_val = -1;
				goto EXIT_LABEL;
			}
		}
		INSERT_BEFORE(job->node, *list);
		jobs_read++;

		next_job_arrival_time = job->arrival_time;

		/* Check if others job also needs to be enqueued. Else, put it on hold. */
		while(jobs_read < GLOBAL.num_jobs)
		{
			if(getline(&job_line, &length, GLOBAL.job_file) == -1)
			{
				ERROR("Error reading line from file.");
				ret_val = -1;
				goto EXIT_LABEL;
			}

			job = (JOB *)malloc(sizeof(JOB));
			if(job==NULL)
			{
				ERROR("Error allocating memory for job schedule");
				ret_val = -1;
				goto EXIT_LABEL;
			}
			INIT_CLL_NODE(job->node, job);
			job->owner_cpu = -1;
			if((params_read = sscanf(job_line, "%d %d %d",&job->pid,
														  &job->arrival_time, 
														  &job->burst_time)) < 3)
			{
				ERROR("Malformed input line. %d", params_read);
				ret_val = -1;
				break;
			}
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


int32_t main(int32_t argc, char *argv[])
{
	int32_t ret_val = 0;

	int32_t ii;

	THREAD_CONTEXT consumers[NUM_COUNTERS];

	GLOBAL.job_file = NULL;

	CLL feed;
	JOB *job=NULL;

	if(argc==2)
	{
		GLOBAL.job_file = fopen(argv[1], "r");
	}
	else
	{
		GLOBAL.job_file = fopen("jobs.txt", "r");
	}
	if(!GLOBAL.job_file)
	{
		ERROR("Could not open job file");
		goto EXIT_LABEL;
	}

	INIT_CLL_ROOT(GLOBAL.queue);
	ret_val = pthread_mutex_init(&GLOBAL.mutex, NULL);
	if(ret_val != 0)
	{
		ERROR("Error initializing mutex.");
		goto EXIT_LABEL;
	}

	GLOBAL.capacity = QUEUE_CAPACITY;
	GLOBAL.occupancy = 0;
	GLOBAL.jobs_in_service = 0;
	sem_init(&GLOBAL.queue_lock, 0, GLOBAL.capacity);

	GLOBAL.ticks = 0;
	GLOBAL.num_jobs = -1;

	for(ii=0;ii<NUM_COUNTERS;ii++)
	{
		consumers[ii].thread_number = ii;
		
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
				if(GLOBAL.occupancy< GLOBAL.capacity)
				{
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
	return(ret_val);
}

void * consume(void *args)
{
	int32_t thread_number = *(int32_t *)args;
	JOB *job = NULL;

	TRACE("Created thread %d", thread_number);

	while(1)
	{
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
				GLOBAL.jobs_in_service +=1 ;
				job->owner_cpu = thread_number;
				pthread_mutex_unlock(&GLOBAL.mutex);
				usleep(job->burst_time*1000);
				pthread_mutex_lock(&GLOBAL.mutex);
				REMOVE_FROM_LIST(job->node);
				GLOBAL.occupancy -=1;
				GLOBAL.jobs_in_service -= 1;
				free(job);
				job = NULL;
			}
		}
		pthread_mutex_unlock(&GLOBAL.mutex);
		/* Else wait on some job to arrive! */

	}

	return(NULL);
}