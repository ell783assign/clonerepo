#define INCLUDE_GLOBALS
#include <include.h>

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
			reset_sink();
			print_output_stat(dispatcher.num_jobs, (JOB *)NULL);
			gantt_headers();/* Opening Headers */
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

int32_t init_dispatcher(char *file_name)
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

void reset_sink()
{
	times_called_global = 0;
	init_time_global = 0;
	iteration_global = 0;
	num_jobs_global = 0;
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
