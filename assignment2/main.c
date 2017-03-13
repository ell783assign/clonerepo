#define INCLUDE_GLOBALS
#include <include.h>
#include <scheduler.h>


int32_t init_dispatcher(const char *);
int32_t init_scheduler();
int32_t init_sink();
void init_scheduler_comn(uint32_t , uint32_t , Feed_Jobs , JOB_SCHEDULER_COMN *);

static inline void show_menu()
{
	int32_t i;
	for(i=0;i<__MAX_SCHEDULER_COUNT__;i++)
	{
		fprintf(stdout, "[%d] %s\n", menu[i].index, menu[i].name);
	}
	fprintf(stdout, "[%d] Exit\n", i);
}

int32_t main(uint32_t argc, char *argv[])
{
	char *job_file;
	int32_t ret_val = 0;

	int32_t scheduler_choice;

	if(argc < 2)
	{
		job_file = "Sample_testcase.txt";
	}
	else
	{
		job_file = argv[1];
	}

	ret_val = init_dispatcher(job_file);
	if(ret_val!= 0)
	{
		ERROR("Error occurred while initializing dispatcher!\n");
		goto EXIT_LABEL;
	}

	ret_val = init_scheduler();
	if(ret_val!= 0)
	{
		ERROR("Error occurred while initializing scheduler!\n");
		goto EXIT_LABEL;
	}

	ret_val = init_sink();
	if(ret_val!= 0)
	{
		ERROR("Error occurred while initializing messaging sink!\n");
		goto EXIT_LABEL;
	}

	/* Now, take input from the user on what scheduler he wants to use. */
	do
	{
		show_menu();
		scheduler_choice = getchar();
		TRACE("You entered %c\n", scheduler_choice);

	}while(scheduler_choice != '8');

	TRACE("Bye Bye!\n");
EXIT_LABEL:
	return(ret_val);
}



int32_t init_dispatcher(const char *file_name)
{
	int32_t ret_val = 0;

	FILE *fp = fopen(file_name, "r");

	char *job_line;
	long unsigned int length;

	JOB *job = NULL;
	
	int32_t ii=0;
	int32_t jobs_read=0;
	int32_t params_read = 0;

	INIT_CLL_ROOT(dispatcher.job_list_root);
	dispatcher.num_jobs_remaining = 0;
	dispatcher.num_jobs = 0;

	if(fp== NULL)
	{
		ERROR("Error opening file.\n");
		goto EXIT_LABEL;
	}

	if(getline(&job_line, &length, fp) <-1)
	{
		ERROR("Error reading line from file.\n");
		ret_val = -1;
		goto EXIT_LABEL;
	}
	if((params_read=sscanf(job_line, "%d", &dispatcher.num_jobs))<1)
	{
		ERROR("Malformed input file.\n");
		ret_val = -1;
		goto EXIT_LABEL;
	}
	TRACE("Number of jobs: %d\n", dispatcher.num_jobs);

	params_read = 0;

	TRACE("%5s |%10s |%10s |%10s |%10s |\n","PID", "Arrival", "Burst", "Priority", "FG[0/1]");

	for(ii=0;ii<dispatcher.num_jobs;ii++)
	{
		free(job_line);
		job_line = NULL;
		if(getline(&job_line, &length, fp) == -1)
		{
			ERROR("Error reading line from file.\n");
			ret_val = -1;
			goto EXIT_LABEL;
		}

		job = (JOB *)malloc(sizeof(JOB));
		if(job==NULL)
		{
			ERROR("Error allocating memory for job schedule\n");
			ret_val = -1;
			goto EXIT_LABEL;
		}
		INIT_CLL_NODE(job->node, job);
		if((params_read = sscanf(job_line, "%d %d %d %d %d\n", &job->pid, &job->arrival_time, 
					&job->burst_time, &job->priority, &job->is_background)) < 5)
		{
			ERROR("Malformed input line. %d\n", params_read);
			ret_val = -1;
			break;
		}
		TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
					job->burst_time, job->priority, job->is_background);

		dispatcher.num_jobs_remaining++;
		
		job->finish_time = -1;
		job->ts_root.next = NULL;
		job->ts_root.ts = -1;

		INSERT_AFTER(job->node, dispatcher.job_list_root);
	}
	free(job_line);
	job_line = NULL;

EXIT_LABEL:	
	return(ret_val);
}

int32_t init_sink()
{
	return 0;
}

int32_t init_scheduler()
{
	scheduler.clock_scheduler.ticks = 0;

	/* initialize each policy scheduler */
	init_scheduler_comn(FCFS, 0, feed_fcfs, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.fcfs);

	init_scheduler_comn(SJF_NP, 0, feed_sjf_np, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.sjf_np);

	init_scheduler_comn(SJF_P, 0, feed_sjf, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.sjf);

	init_scheduler_comn(RR, 10, feed_rr, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.rr);

	init_scheduler_comn(PRIORITY, 0, feed_prio, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.prio);

	init_scheduler_comn(MULTILEVEL_Q, 0, feed_ml, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.ml);

	init_scheduler_comn(MF_Q, 0, feed_mfq, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.ml_fb);

	init_scheduler_comn(CFS, 0, feed_cfs, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.cfs);
		
	return 0;
}

void init_scheduler_comn(uint32_t policy, uint32_t slice_size, Feed_Jobs method, JOB_SCHEDULER_COMN *sched)
{
	sched->feeder = method;
	sched->time_slice = slice_size;
	sched->policy = policy;

	INIT_CLL_ROOT(sched->pending_jobs_queue);
	sched->job_in_service = NULL;
}


void feed_fcfs(JOB *job)
{

}
void feed_sjf_np(JOB *job)
{

}
void feed_sjf(JOB *job)
{

}
void feed_rr(JOB *job)
{

}
void feed_prio(JOB *job)
{

}
void feed_ml(JOB *job)
{

}
void feed_mfq(JOB *job)
{

}
void feed_cfs(JOB *job)
{

}