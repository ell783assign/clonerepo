#define INCLUDE_GLOBALS
#include <include.h>
#include <scheduler.h>


int32_t init_dispatcher(const char *);
int32_t init_scheduler();
void reset_dispatcher();
int32_t init_sink();
void init_scheduler_comn(uint32_t , uint32_t , Feed_Jobs , JOB_SCHEDULER_COMN *);
int32_t get_jobs_at_instant(uint32_t , CLL *);

void spin_scheduler();

void insertion_sort_insert(JOB *, CLL *, long int , uint32_t);

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
	int32_t garbage_collection;

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
		scheduler_choice = fgetc(stdin);
		switch(scheduler_choice)
		{
			case '0':
				scheduler.job_scheduler.current_scheduler = FCFS;
				break;
			case '1':
				scheduler.job_scheduler.current_scheduler = SJF_NP;
				break;
			case '2':
				scheduler.job_scheduler.current_scheduler = RR;
				break;
			case '3':
				scheduler.job_scheduler.current_scheduler = SJF_P;
				break;
			case '4':
				scheduler.job_scheduler.current_scheduler = PRIORITY;
				break;
			case '5':
				scheduler.job_scheduler.current_scheduler = MULTILEVEL_Q;
				break;
			case '6':
				scheduler.job_scheduler.current_scheduler = MF_Q;
				break;
			case '7':
				scheduler.job_scheduler.current_scheduler = CFS;
				break;
			default:
				scheduler.job_scheduler.current_scheduler = -1;
				break;
		}

		if(scheduler.job_scheduler.current_scheduler>=0 && scheduler.job_scheduler.current_scheduler < __MAX_SCHEDULER_COUNT__)
		{
			reset_dispatcher();
			spin_scheduler();
		}

		/* flush stdin */
		if(scheduler_choice!='\n')
		{
			garbage_collection = fgetc(stdin);
			while(garbage_collection != '\n')
			{
				garbage_collection = fgetc(stdin);
			}
		}

	}while(scheduler_choice != '8');

	TRACE("Bye Bye!\n");
EXIT_LABEL:
	return(ret_val);
}

int32_t init_dispatcher(const char *file_name)
{
	int32_t ret_val = 0;

	char *job_line;
	long unsigned int length;

	JOB *job = NULL;
	
	int32_t ii=0;
	int32_t jobs_read=0;
	int32_t params_read = 0;

	INIT_CLL_ROOT(dispatcher.job_list_root);
	dispatcher.num_jobs_remaining = 0;
	dispatcher.num_jobs = 0;
	dispatcher.file_name = file_name;

EXIT_LABEL:	
	return(ret_val);
}

void reset_dispatcher()
{
	int32_t ret_val = 0;

	TRACE("Opening %s\n", dispatcher.file_name);
	FILE *fp = fopen(dispatcher.file_name, "r");

	char *job_line=NULL;
	long unsigned int length;

	JOB *job = NULL;
	
	int32_t ii=0;
	int32_t jobs_read=0;
	int32_t params_read = 0;

	dispatcher.num_jobs_remaining = 0;
	dispatcher.num_jobs = 0;

	/* Free any leftover jobs from previous run */
	job = (JOB *)NEXT_IN_LIST(dispatcher.job_list_root);
	while(job!=NULL)
	{
		REMOVE_FROM_LIST(job->node);
		free(job);
		job = (JOB *)NEXT_IN_LIST(dispatcher.job_list_root);
	}

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

	for(ii=0;ii<dispatcher.num_jobs;ii++)
	{
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
					&job->burst_time, &job->priority, &job->is_foreground)) < 5)
		{
			ERROR("Malformed input line. %d\n", params_read);
			ret_val = -1;
			break;
		}
		
		dispatcher.num_jobs_remaining++;
		
		job->finish_time = -1;
		job->start_time = -1;
		job->run_time = 0;
		job->remaining_time = job->burst_time;
		INIT_CLL_ROOT(job->ts_root);

		INSERT_BEFORE(job->node, dispatcher.job_list_root);
	}

	free(job_line);
	job_line = NULL;

	TRACE("%5s |%10s |%10s |%10s |%10s |\n","PID", "Arrival", "Burst", "Priority", "FG[0/1]");
	for(job = (JOB *)NEXT_IN_LIST(dispatcher.job_list_root);
		job != NULL;
		job = (JOB *)NEXT_IN_LIST(job->node))
	{
		TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
					job->burst_time, job->priority, job->is_foreground);
	}

	fclose(fp);

EXIT_LABEL:	
	return;
}

int32_t init_sink()
{
	return 0;
}

int32_t init_scheduler()
{
	scheduler.clock_scheduler.ticks = 0;
	char *slice_size = NULL;
	char *max_age_val = NULL;
	int32_t slice = 1;

	int32_t max_age = 7;

	JOB *job = NULL;
	int32_t i;

	slice_size = getenv("SCHED_SLICE_SIZE");
	if(slice_size != NULL)
	{
		slice = atoi(slice_size);
	}
	TRACE("Slice Size=%d\n", slice);

	max_age_val = getenv("SCHED_MAX_AGE_MULTIPLIER");
	if(max_age_val!=NULL)
	{
		max_age = atoi(max_age_val);
	}
	TRACE("Max Age Multiplier=%d\n", max_age);

	/* initialize each policy scheduler */
	init_scheduler_comn(FCFS, slice, feed_fcfs, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.fcfs);

	init_scheduler_comn(SJF_NP, slice, feed_sjf_np, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.sjf_np);

	init_scheduler_comn(SJF_P, slice, feed_sjf, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.sjf);

	init_scheduler_comn(RR, slice, feed_rr, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.rr);

	init_scheduler_comn(PRIORITY, slice, feed_prio, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.prio);

	init_scheduler_comn(MULTILEVEL_Q, slice, feed_ml, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.ml);
	INIT_CLL_ROOT(scheduler.job_scheduler.ml.foreground);
	INIT_CLL_ROOT(scheduler.job_scheduler.ml.background);

	init_scheduler_comn(MF_Q, slice, feed_mfq, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.ml_fb);
	INIT_CLL_ROOT(scheduler.job_scheduler.ml_fb.first);
	INIT_CLL_ROOT(scheduler.job_scheduler.ml_fb.second);
	scheduler.job_scheduler.ml_fb.age_list = (JOB **)malloc(sizeof(JOB)*max_age);
	scheduler.job_scheduler.ml_fb.max_age = max_age;
	for(i=0;i<max_age;i++)
	{
		scheduler.job_scheduler.ml_fb.age_list[i]  =NULL;
	}
	scheduler.job_scheduler.ml_fb.current_index = 0;
	/* At each instant, we check at (current_index+max_age-1)%max_age slot if a process is present */


	init_scheduler_comn(CFS, slice, feed_cfs, (JOB_SCHEDULER_COMN *)&scheduler.job_scheduler.cfs);
	BST_TREE_INIT(scheduler.job_scheduler.cfs.tree, compare_int, offsetof(JOB, vruntime));
		
	return 0;
}

void init_scheduler_comn(uint32_t policy, uint32_t slice_size, Feed_Jobs method, JOB_SCHEDULER_COMN *sched)
{
	sched->feeder = method;
	sched->time_slice = slice_size;
	sched->policy = policy;
	sched->next_switch = slice_size; /* Next context switch occurs at time slice period, if applicable*/

	INIT_CLL_ROOT(sched->pending_jobs_queue);
	sched->job_in_service = NULL;
}


void feed_fcfs(CLL * incoming_job_queue)
{
	JOB *job=NULL;
	JOB *current_job = NULL;
	TIMESTAMP *ts = NULL;

	/* Put incoming jobs at the end of queue */
	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			INSERT_BEFORE(job->node, scheduler.job_scheduler.fcfs.comn.pending_jobs_queue);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
						job->burst_time, job->priority, job->is_foreground);
		}
	}
	/* There is no preemption, so each job must run to completion */
	current_job = scheduler.job_scheduler.fcfs.comn.job_in_service;
	if(current_job==NULL)
	{
		/* No job in CPU. Pluck one from queue */
		current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.fcfs.comn.pending_jobs_queue);
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
		}
	}
	else
	{
		/* There is a job executing. Has it completed execution? */
		current_job->run_time++;
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.fcfs.comn.pending_jobs_queue);
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);
			}
			/* Else we have nothing to do for now. */
		}
		/* By now, either an IDLE CPU has been fed a job. Or, an already running job continues to run.
		   Or, an already running job finishes execution and next job if available is scheduled. */
	}
	scheduler.job_scheduler.fcfs.comn.job_in_service = current_job;
	return;
}

void feed_sjf_np(CLL *incoming_job_queue)
{
	JOB *job = NULL;
	JOB *current_job = NULL;
	TIMESTAMP *ts = NULL;

	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
			job->burst_time, job->priority, job->is_foreground);
			insertion_sort_insert(job, &scheduler.job_scheduler.sjf_np.comn.pending_jobs_queue, offsetof(JOB, remaining_time), FALSE);
		}
	}

	current_job = scheduler.job_scheduler.sjf_np.comn.job_in_service;
	/* There is no time-slicing */
	if(current_job==NULL)
	{
		/* No job in CPU. Pluck one from queue: Here, we traverse the list backwards */
		current_job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.sjf_np.comn.pending_jobs_queue);
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
		}
	}
	else
	{
		/* There is a job executing. Has it completed execution? */
		current_job->run_time++;
		current_job->remaining_time--;
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.sjf_np.comn.pending_jobs_queue);
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);
			}
			/* Else we have nothing to do for now. */
		}
	}
	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.sjf_np.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.sjf_np.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}
	return;
}

void feed_sjf(CLL *incoming_job_queue)
{
	JOB *job = NULL;
	JOB *current_job = NULL;
	TIMESTAMP *ts = NULL;

	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
			job->burst_time, job->priority, job->is_foreground);
			insertion_sort_insert(job, &scheduler.job_scheduler.sjf.comn.pending_jobs_queue, offsetof(JOB, remaining_time), FALSE);
		}
	}

	current_job = scheduler.job_scheduler.sjf.comn.job_in_service;
	/* There is no time-slicing */
	if(current_job==NULL)
	{
		/* No job in CPU. Pluck one from queue: Here, we traverse the list backwards */
		current_job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.sjf.comn.pending_jobs_queue);
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
		}
	}
	else
	{
		/* There is a job executing. Has it completed execution? */
		current_job->run_time++;
		current_job->remaining_time--;
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.sjf.comn.pending_jobs_queue);
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);
			}
			/* Else we have nothing to do for now. */
		}
		else
		{
			job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.sjf.comn.pending_jobs_queue);
			if( job!=NULL && current_job->remaining_time > job->remaining_time)
			{
				/* Swap out currently executing job for a job with lesser remaining time (saved in current_job)*/
				insertion_sort_insert(current_job, 
										&scheduler.job_scheduler.sjf.comn.pending_jobs_queue,
										offsetof(JOB, remaining_time), FALSE);
				TRACE("Swap %d out at %d\n", current_job->pid, scheduler.clock_scheduler.ticks);

				current_job = job;

				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Swap %d in\n", current_job->pid);
			}
		}
	}
	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.sjf.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.sjf.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}
	return;
}
void feed_rr(CLL *incoming_job_queue)
{
	JOB *job=NULL;
	JOB *current_job = NULL;

	TIMESTAMP *ts = NULL;

	/* Put incoming jobs at the end of queue */
	/**
	 * This assumes that all jobs have arrived while the current job was 
	 * executing.
	 * This means that while we are checking if the current job has finished
	 * its quota, no new jobs may be added to the queue.
	 */
	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			INSERT_BEFORE(job->node, scheduler.job_scheduler.rr.comn.pending_jobs_queue);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
						job->burst_time, job->priority, job->is_foreground);
		}
	}
	/* Is there a job finishing in this time instant? */
	current_job = scheduler.job_scheduler.rr.comn.job_in_service;
	if(current_job==NULL)
	{
		/* IDLE CPU. Find next job if available */
				/* Pop next job from queue and update its service in time. */
		current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.rr.comn.pending_jobs_queue);
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
			/* Update next switch time to either current_time+slice*/
			scheduler.job_scheduler.rr.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.rr.comn.time_slice;
		}
	}
	else
	{
		current_job->run_time++;
		/* Has the current job finished executing? */
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.rr.comn.pending_jobs_queue);
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);
				/* Update next switch time to either current_time+slice*/
				scheduler.job_scheduler.rr.comn.next_switch = 
										scheduler.clock_scheduler.ticks + 
										scheduler.job_scheduler.rr.comn.time_slice;
			}
		}
		/* Else, have we exceeded out time slice? */
		else if(scheduler.job_scheduler.rr.comn.next_switch 
										== scheduler.clock_scheduler.ticks)
		{
			/* Swap it out. Move it to the end of pending queue if there is another process which is waiting */
			if(NEXT_IN_LIST(scheduler.job_scheduler.rr.comn.pending_jobs_queue)!=NULL)
			{
				INSERT_BEFORE(current_job->node, 
							scheduler.job_scheduler.rr.comn.pending_jobs_queue);

				TRACE("Swap %d out at %d\n", current_job->pid, scheduler.clock_scheduler.ticks);

				/* Pop next job from queue and update its service in time. */
				current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.rr.comn.pending_jobs_queue);
				if(current_job!=NULL)
				{
					REMOVE_FROM_LIST(current_job->node);
					ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
					if(ts==NULL)
					{
						ERROR("Error allocating memory for preserving state of job.\n");
						exit(0);
					}
					ts->ts = scheduler.clock_scheduler.ticks;
					INIT_CLL_NODE(ts->node, ts);
					INSERT_BEFORE(ts->node, current_job->ts_root);
					if(current_job->start_time==-1)
					{
						/* Scheduled for the first time */
						current_job->start_time = scheduler.clock_scheduler.ticks;
						current_job->run_time = 0;
					}
					TRACE("Swap %d in\n", current_job->pid);
				}
				/* Update next switch time to either current_time+slice*/
				scheduler.job_scheduler.rr.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.rr.comn.time_slice;
			}
		}
		/* Else continue doing what we were doing. */
	}

	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.rr.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.rr.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}

	return;
}

void insertion_sort_insert(JOB *job, CLL *queue, long int key_offset, uint32_t do_fcfs)
{
	/* Always sort in ascending order. */
	JOB *node = (JOB *)NEXT_IN_LIST(*queue);
	int32_t inserted = FALSE;
	while(node != NULL)
	{
		if(do_fcfs)
		{
			/* On tasks with same metric, order is first come first served */
			if((int32_t)*((char *)job + key_offset) > (int32_t)*((char *)node + key_offset))
			{
				TRACE("Insert before job %d\n", node->pid);
				INSERT_BEFORE(job->node, node->node);
				inserted = TRUE;
				break;
			}
		}
		else
		{
			/* On tasks with same metric, order is last come first served. This is used in SJF when traversing the list backwards */
			if((int32_t)*((char *)job + key_offset) >= (int32_t)*((char *)node + key_offset))
			{
				TRACE("Insert before job %d\n", node->pid);
				INSERT_BEFORE(job->node, node->node);
				inserted = TRUE;
				break;
			}	
		}
		node = (JOB *)NEXT_IN_LIST(node->node);
	}
	if(!inserted)
	{
		TRACE("Insert at end of queue\n");
		INSERT_BEFORE(job->node, *queue);
	}
	return;
}

void feed_prio(CLL *incoming_job_queue)
{
	JOB *job = NULL;
	JOB *current_job = NULL;
	TIMESTAMP *ts = NULL;
	/* 
	 * The already present pending queue is already sorted by priority.
	 * So, we simply use insertion sort to place each job from the incoming
	 * queue in its right place.
	 * 
	 * Finally, if the currently running job has priority less than the job at 
	 * the head of pending queue, pre-empt it and add it to the queue after
	 * appropriately updating its runtime.
	 */

	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
			job->burst_time, job->priority, job->is_foreground);
			insertion_sort_insert(job, &scheduler.job_scheduler.prio.comn.pending_jobs_queue, offsetof(JOB, priority), FALSE);
		}
	}

	current_job = scheduler.job_scheduler.prio.comn.job_in_service;
	/* There is no time-slicing */
	if(current_job==NULL)
	{
		/* No job in CPU. Pluck one from queue */
		/* Since the list is ordered in descending order, we read it from behind. */
		current_job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.prio.comn.pending_jobs_queue);
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
		}
	}
	else
	{
		/* There is a job executing. Has it completed execution? */
		current_job->run_time++;
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.prio.comn.pending_jobs_queue);
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);
			}
			/* Else we have nothing to do for now. */
		}
		else
		{
			job = (JOB *)PREV_IN_LIST(scheduler.job_scheduler.prio.comn.pending_jobs_queue);
			if( job!=NULL && current_job->priority > job->priority)
			{
				insertion_sort_insert(current_job, 
										&scheduler.job_scheduler.prio.comn.pending_jobs_queue,
										offsetof(JOB, priority), FALSE);
				TRACE("Swap %d out at %d\n", current_job->pid, scheduler.clock_scheduler.ticks);

				current_job = job;

				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Swap %d in\n", current_job->pid);
			}
		}
	}
	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.prio.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.prio.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}

	return;
}

void feed_ml(CLL *incoming_job_queue)
{
	JOB *job = NULL;
	JOB *current_job = NULL;
	TIMESTAMP *ts = NULL;

	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			if(!job->is_foreground)
			{
				INSERT_BEFORE(job->node, scheduler.job_scheduler.ml.background);
			}
			else
			{
				INSERT_BEFORE(job->node, scheduler.job_scheduler.ml.foreground);
			}
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
						job->burst_time, job->priority, job->is_foreground);
		}
	}

	current_job = scheduler.job_scheduler.ml.comn.job_in_service;
	if(current_job==NULL)
	{
		/* No currently executing job. Fetch next available job. */
		current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml.foreground);
		if(current_job==NULL)
		{
			/* No job in high priority queue. Try background queue. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml.background);
		}
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
			/* Update next switch time to either current_time+slice*/
			/* This will be applicable only if the round robin queue is in service */
			scheduler.job_scheduler.ml.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.ml.comn.time_slice;
		}
		/* Else there is no job and the scheduler is idle */
	}
	else if(current_job != NULL)/* There is some process executing */
	{
		current_job->run_time++;
		/* Has it finished executing? */
		if(current_job->run_time == current_job->burst_time)
		{
			/* Yep. Remove it and schedule the next eleigible task. */
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml.foreground);
			if(current_job==NULL)
			{
				current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml.background);
			}
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);

				/* Update next switch time to either current_time+slice*/
				/* This will be applicable only if the round robin queue is in service */
				scheduler.job_scheduler.ml.comn.next_switch = 
										scheduler.clock_scheduler.ticks + 
										scheduler.job_scheduler.ml.comn.time_slice;

			}
		}
		/* If currently running job is a foreground task, it is round robined */
		/* Has the time slice expired? */
		else if(current_job->is_foreground && scheduler.job_scheduler.ml.comn.next_switch 
																== scheduler.clock_scheduler.ticks)
		{
			/* Swap it out. Move it to the end of pending queue only if there is some other job */
			if(NEXT_IN_LIST(scheduler.job_scheduler.ml.foreground)!=NULL)
			{
				INSERT_BEFORE(current_job->node, 
							scheduler.job_scheduler.ml.foreground);

				TRACE("Swap %d out at %d\n", current_job->pid, scheduler.clock_scheduler.ticks);

				/* Pop next job from queue and update its service in time. */
				current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml.foreground);
				if(current_job!=NULL)
				{
					REMOVE_FROM_LIST(current_job->node);
					ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
					if(ts==NULL)
					{
						ERROR("Error allocating memory for preserving state of job.\n");
						exit(0);
					}
					ts->ts = scheduler.clock_scheduler.ticks;
					INIT_CLL_NODE(ts->node, ts);
					INSERT_BEFORE(ts->node, current_job->ts_root);
					if(current_job->start_time==-1)
					{
						/* Scheduled for the first time */
						current_job->start_time = scheduler.clock_scheduler.ticks;
						current_job->run_time = 0;
					}
					TRACE("Swap %d in\n", current_job->pid);
				}
			}
			/* Update next switch time to either current_time+slice. This happens irrespective of whether the job was 
			 swapped out or not. */
			scheduler.job_scheduler.ml.comn.next_switch = 
								scheduler.clock_scheduler.ticks + 
								scheduler.job_scheduler.ml.comn.time_slice;

		}
		/* Or it is a background task, but a new foreground task has arrived */
		else if(!current_job->is_foreground && NEXT_IN_LIST(scheduler.job_scheduler.ml.foreground)!=NULL)
		{
			/* Preempt the background task and let the foreground task run */
			INSERT_AFTER(current_job->node, scheduler.job_scheduler.ml.background);
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml.foreground);

			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			if(current_job->start_time==-1)
			{
				/* Scheduled for the first time */
				current_job->start_time = scheduler.clock_scheduler.ticks;
				current_job->run_time = 0;
			}
			TRACE("Schedule %d \n", current_job->pid);
			/* Update next switch time to either current_time+slice*/
			scheduler.job_scheduler.ml.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.ml.comn.time_slice;
		}
		else
		{
			/* It was a foreground job and it has neither finished nor exceeded its time quanta */
			/* Or, it was a background task and is still executing. */
		}
	}

	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.ml.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.ml.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}

	return;
}

void feed_mfq(CLL *incoming_job_queue)
{
	JOB *job = NULL;
	JOB *current_job = NULL;
	TIMESTAMP *ts = NULL;
	int32_t current_index;
	static first_queue_in_use = TRUE; /* To save where my current job came from, to decide its scheduling */
	int32_t max_age = scheduler.job_scheduler.ml_fb.max_age;

	int32_t i;

	/* Assuming for now that both queues get same time quanta */
	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			/* Everything first gets into the first queue and is run at least once. */
			INSERT_BEFORE(job->node, scheduler.job_scheduler.ml_fb.first);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
						job->burst_time, job->priority, job->is_foreground);
		}
	}
	/* If some process in the age list is exceeding threshold, move it to the end of higher queue */
	/**
	 * This is slightly skewed, because the effective time before it should have moved might have
	 * been shortened in case there were tasks that took time less than time_slice.
	 */
	if(scheduler.job_scheduler.ml_fb.comn.next_switch 
												== scheduler.clock_scheduler.ticks)
	{
		current_index = scheduler.job_scheduler.ml_fb.current_index;
		//TRACE("Check at %d\n", (current_index + max_age -1)%max_age);
		if(scheduler.job_scheduler.ml_fb.age_list[(current_index + max_age -1)%max_age] != NULL)
		{
			/* Move this job to end of higher level queue. */
			job = scheduler.job_scheduler.ml_fb.age_list[(current_index + max_age -1)%max_age];
			REMOVE_FROM_LIST(job->node);
			INSERT_BEFORE(job->node, scheduler.job_scheduler.ml_fb.first);
			scheduler.job_scheduler.ml_fb.age_list[(current_index + max_age -1)%max_age] = NULL;
			TRACE("Move %d into higher priority queue.\n", job->pid);
		}
		/* Move the current position one step ahead */
		scheduler.job_scheduler.ml_fb.current_index = (scheduler.job_scheduler.ml_fb.current_index+1)%max_age;
	}

	current_job = scheduler.job_scheduler.ml_fb.comn.job_in_service;
	if(current_job==NULL)
	{
		/* No currently executing job. Fetch next available job. */
		//TRACE("CPU Idle\n");
		current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.first);
		if(current_job==NULL)
		{
			/* No job in high priority queue. Try background queue. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.second);
			if(current_job)
			{
				first_queue_in_use = FALSE; /* We're working with second queue. */
			}			
		}
		if(current_job!=NULL)
		{
			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			current_job->start_time = scheduler.clock_scheduler.ticks;
			current_job->run_time = 0;

			TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
			/* Update next switch time to either current_time+slice*/
			scheduler.job_scheduler.ml_fb.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.ml_fb.comn.time_slice;
		}
		/* Else there is no job and the scheduler is idle */
	}
	else if(current_job != NULL)/* There is some process executing */
	{
		/* Has it finished processing? */
		current_job->run_time++;
		/* Has it finished executing? */
		if(current_job->run_time == current_job->burst_time)
		{
			/* Yep. Remove it and schedule the next eleigible task. */
			/* Pass it to sink module */
			//TODO
			current_job->finish_time = scheduler.clock_scheduler.ticks;
			TRACE("Job %d finished at %d\n", current_job->pid, current_job->finish_time);
			/* Remove its time stamps */
			ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			while(ts != NULL)
			{
				//TRACE("Scheduled at %d\n", ts->ts);
				REMOVE_FROM_LIST(ts->node);
				free(ts);
				ts = (TIMESTAMP *)NEXT_IN_LIST(current_job->ts_root);
			}			
			free(current_job);

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.first);
			if(current_job==NULL)
			{
				current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.second);
				if(current_job)
				{
					first_queue_in_use = FALSE; /* We're working with second queue. */
					/* Also because it has been scheduled, if it exists in the age list, remove */
					for(i=0;i<scheduler.job_scheduler.ml_fb.max_age;i++)
					{
						if(scheduler.job_scheduler.ml_fb.age_list[i]== current_job)
						{
							scheduler.job_scheduler.ml_fb.age_list[i] = NULL;	
						}
					}
				}	
			}
			else
			{
				first_queue_in_use = TRUE;
			}
			if(current_job!=NULL)
			{
				REMOVE_FROM_LIST(current_job->node);
				ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
				if(ts==NULL)
				{
					ERROR("Error allocating memory for preserving state of job.\n");
					exit(0);
				}
				ts->ts = scheduler.clock_scheduler.ticks;
				INIT_CLL_NODE(ts->node, ts);
				INSERT_BEFORE(ts->node, current_job->ts_root);
				if(current_job->start_time==-1)
				{
					/* Scheduled for the first time */
					current_job->start_time = scheduler.clock_scheduler.ticks;
					current_job->run_time = 0;
				}
				TRACE("Schedule %d \n", current_job->pid);

				/* Update next switch time to either current_time+slice*/
				/* This will be applicable only if the round robin queue is in service */
				scheduler.job_scheduler.ml_fb.comn.next_switch = 
										scheduler.clock_scheduler.ticks + 
										scheduler.job_scheduler.ml_fb.comn.time_slice;

			}
		}
		/* If currently running job is a first queue task, it is round robined 
		 * If time slice has expired, then we swap it out for another task in the
		 * higher priority queue. We ignore lower priority queue here.
		 */
		else if(first_queue_in_use && scheduler.job_scheduler.ml_fb.comn.next_switch 
																== scheduler.clock_scheduler.ticks)
		{
			/* Swap it out. Move it to the end of appropriate queue only if there is some other job */
			//TRACE("Slice expired.\n");
			if(NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.first)!=NULL)
			{
				first_queue_in_use = TRUE;

				if(current_job->is_foreground)
				{
					INSERT_BEFORE(current_job->node, 
								scheduler.job_scheduler.ml_fb.first);
				}
				else
				{
					/* After getting first chunk, if process was of background kind, move it to second queue */
					INSERT_BEFORE(current_job->node, 
								scheduler.job_scheduler.ml_fb.second);	
					/* Also put it in age list for possible promotion */				
					scheduler.job_scheduler.ml_fb.age_list[scheduler.job_scheduler.ml_fb.current_index] = current_job;
				}

				TRACE("Swap %d out at %d\n", current_job->pid, scheduler.clock_scheduler.ticks);

				/* Pop next job from queue and update its service in time. */
				current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.first);
				if(current_job!=NULL)
				{
					REMOVE_FROM_LIST(current_job->node);
					ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
					if(ts==NULL)
					{
						ERROR("Error allocating memory for preserving state of job.\n");
						exit(0);
					}
					ts->ts = scheduler.clock_scheduler.ticks;
					INIT_CLL_NODE(ts->node, ts);
					INSERT_BEFORE(ts->node, current_job->ts_root);
					if(current_job->start_time==-1)
					{
						/* Scheduled for the first time */
						current_job->start_time = scheduler.clock_scheduler.ticks;
						current_job->run_time = 0;
					}
					TRACE("Swap %d in\n", current_job->pid);
				}
			}
			/* Else, we let it run more */
			/* Update next switch time to either current_time+slice. This happens irrespective of whether the job was 
			 swapped out or not. */
			scheduler.job_scheduler.ml_fb.comn.next_switch = 
								scheduler.clock_scheduler.ticks + 
								scheduler.job_scheduler.ml_fb.comn.time_slice;

		}
		/* Or it is a lower queue task. 
		   Lower queue is FCFS. But higher queue pre-empts it. 
		   So, if there is a job in higher priority queue, preempt the job.
		   A higher queue may contain tasks if a new task arrived, or
		   an aged task was promoted to the higher queue.
		*/
		else if(!first_queue_in_use && NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.first)!=NULL)
		{
			/* Preempt the background task and let the foreground task run */
			INSERT_AFTER(current_job->node, scheduler.job_scheduler.ml_fb.second);
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.ml_fb.first);

			first_queue_in_use = TRUE;

			REMOVE_FROM_LIST(current_job->node);
			ts = (TIMESTAMP *)malloc(sizeof(TIMESTAMP));
			if(ts==NULL)
			{
				ERROR("Error allocating memory for preserving state of job.\n");
				exit(0);
			}
			ts->ts = scheduler.clock_scheduler.ticks;
			INIT_CLL_NODE(ts->node, ts);
			INSERT_BEFORE(ts->node, current_job->ts_root);
			if(current_job->start_time==-1)
			{
				/* Scheduled for the first time */
				current_job->start_time = scheduler.clock_scheduler.ticks;
				current_job->run_time = 0;
			}
			TRACE("Schedule %d \n", current_job->pid);
			/* Update next switch time to either current_time+slice*/
			scheduler.job_scheduler.ml.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.ml.comn.time_slice;
		}
		else
		{
			/* It was a foreground job and it has neither finished nor exceeded its time quanta */
			/* Or, it was a background task and is still executing. */
		}	
	}

	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.ml_fb.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.ml_fb.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}

	return;
}
void feed_cfs(CLL *incoming_job_queue)
{

}

int32_t get_jobs_at_instant(uint32_t instant, CLL *root)
{
	int32_t num_jobs = 0;
	JOB *next_job = NEXT_IN_LIST(dispatcher.job_list_root);

	if(next_job != NULL)
	{
		while(next_job != NULL && next_job->arrival_time==instant)
		{
			/* Remove it from input jobs queue and add it to the list passed. */
			REMOVE_FROM_LIST(next_job->node);
			INSERT_BEFORE(next_job->node, *root);
			next_job = NEXT_IN_LIST(dispatcher.job_list_root);		
			dispatcher.num_jobs_remaining--;
			num_jobs++;
		}
	}
	else
	{
		/* Signal we are finished. */
		//TRACE("No more jobs.\n");
		num_jobs = -1;
	}

	return(num_jobs);
}

void spin_scheduler()
{
	JOB_SCHEDULER_COMN *policy = NULL;
	uint32_t num_jobs_added = 0;

	CLL root;
	scheduler.clock_scheduler.ticks = 0;

	scheduler.clock_scheduler.signal_stop = FALSE;

	switch(scheduler.job_scheduler.current_scheduler)
	{
		case FCFS:
			TRACE("First Come First Served.\n");
			policy = ( JOB_SCHEDULER_COMN * ) &scheduler.job_scheduler.fcfs;
			break;

		case SJF_NP:
			TRACE("Shortest Job First (Non-Preemptive).\n");
			policy = ( JOB_SCHEDULER_COMN * ) &scheduler.job_scheduler.sjf_np;
			break;

		case RR:
			TRACE("Round Robin.\n");
			policy = ( JOB_SCHEDULER_COMN * ) &scheduler.job_scheduler.rr;
			break;			

		case SJF_P:
			TRACE("Shortest Job First (Premptive).\n");
			policy = ( JOB_SCHEDULER_COMN * ) &scheduler.job_scheduler.sjf;
			break;

		case PRIORITY:
			TRACE("Priority.\n");
			policy = ( JOB_SCHEDULER_COMN * )&scheduler.job_scheduler.prio;
			break;

		case MULTILEVEL_Q:
			TRACE("Multilevel Queue.\n");
			policy = ( JOB_SCHEDULER_COMN * )&scheduler.job_scheduler.ml;
			break;

		case MF_Q:
			TRACE("Multilevel with Feedback Queue.\n");
			policy = ( JOB_SCHEDULER_COMN * )&scheduler.job_scheduler.ml_fb;
			break;

		case CFS:
			TRACE("Completely Fair.\n");
			policy = ( JOB_SCHEDULER_COMN * )&scheduler.job_scheduler.cfs;
			break;

		default:
			ERROR("Unknown scheduler.\n");
			exit(0);									
	}
	while(scheduler.clock_scheduler.signal_stop == FALSE)
	{
		INIT_CLL_ROOT(root);
		num_jobs_added = get_jobs_at_instant(scheduler.clock_scheduler.ticks, &root);

		//TRACE("Number of jobs arriving at %d: %d\n", scheduler.clock_scheduler.ticks, num_jobs_added);
		policy->feeder(&root);

		if(num_jobs_added == -1)
		{
			//TRACE("No more jobs in input queue.\n");
			if(policy->job_in_service == NULL)
			{
				scheduler.clock_scheduler.signal_stop = TRUE;
			}
		}
		scheduler.clock_scheduler.ticks++;
	}
	return;
}
