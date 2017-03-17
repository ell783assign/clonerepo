#include <include.h>

static inline void job_completed(JOB *job)
{
	TIMESTAMP *ts = NULL;
	job->finish_time = scheduler.clock_scheduler.ticks;
	TRACE("Job %d finished at %d\n", job->pid, job->finish_time);
	/* Remove its time stamps */
	ts = (TIMESTAMP *)NEXT_IN_LIST(job->ts_root);
	while(ts != NULL)
	{
		//TRACE("Scheduled at %d\n", ts->ts);
		REMOVE_FROM_LIST(ts->node);
		free(ts);
		ts = (TIMESTAMP *)NEXT_IN_LIST(job->ts_root);
	}			
	message_sink(job->pid, job->finish_time, TRUE);

	/* Update Average wait time and second moment */
	waiting_stats.variance = (((waiting_stats.n/(waiting_stats.n +1))*waiting_stats.second_moment) +
								(((job->finish_time - job->arrival_time - job->burst_time)*
										(job->finish_time - job->arrival_time - job->burst_time))/(waiting_stats.n +1)))
							- ((((waiting_stats.n/(waiting_stats.n +1))*waiting_stats.first_moment) +
								(((job->finish_time - job->arrival_time - job->burst_time))/(waiting_stats.n +1)))
								*(((waiting_stats.n/(waiting_stats.n +1))*waiting_stats.first_moment) +
								(((job->finish_time - job->arrival_time - job->burst_time))/(waiting_stats.n +1))));
	waiting_stats.first_moment = ((waiting_stats.n/(waiting_stats.n +1))*waiting_stats.first_moment)
									+ ((job->finish_time - job->arrival_time - job->burst_time)/(waiting_stats.n +1));

	waiting_stats.second_moment =  ((waiting_stats.n/(waiting_stats.n +1))*waiting_stats.second_moment)
									+ (((job->finish_time - job->arrival_time - job->burst_time)*
										(job->finish_time - job->arrival_time - job->burst_time))/(waiting_stats.n +1));
	waiting_stats.n +=1;

	/* Update Average turnaround time and second moment */
	turnaround_stats.variance = (((turnaround_stats.n/(turnaround_stats.n +1))*turnaround_stats.second_moment) +
							(((job->finish_time - job->arrival_time)*
									(job->finish_time - job->arrival_time))/(turnaround_stats.n +1)))
						- ((((turnaround_stats.n/(turnaround_stats.n +1))*turnaround_stats.first_moment) +
							(((job->finish_time - job->arrival_time ))/(turnaround_stats.n +1)))
							*(((turnaround_stats.n/(turnaround_stats.n +1))*turnaround_stats.first_moment) +
							(((job->finish_time - job->arrival_time ))/(turnaround_stats.n +1))));
	turnaround_stats.first_moment = ((turnaround_stats.n/(turnaround_stats.n +1))*turnaround_stats.first_moment)
									+ ((job->finish_time - job->arrival_time)/(turnaround_stats.n +1));

	turnaround_stats.second_moment =  ((turnaround_stats.n/(turnaround_stats.n +1))*turnaround_stats.second_moment)
									+ (((job->finish_time - job->arrival_time)*
										(job->finish_time - job->arrival_time))/(turnaround_stats.n +1));
	turnaround_stats.n +=1;

	/* Update Average running time and second moment */
	running_stats.variance = (((running_stats.n/(running_stats.n +1))*running_stats.second_moment) +
							(((job->burst_time)*
									(job->burst_time))/(running_stats.n +1)))
						- ((((running_stats.n/(running_stats.n +1))*running_stats.first_moment) +
							(((job->burst_time ))/(running_stats.n +1)))
							*(((running_stats.n/(running_stats.n +1))*running_stats.first_moment) +
							(((job->burst_time ))/(running_stats.n +1))));	
	running_stats.first_moment = ((running_stats.n/(running_stats.n +1))*running_stats.first_moment)
									+ ((job->burst_time)/(running_stats.n +1));

	running_stats.second_moment =  ((running_stats.n/(running_stats.n +1))*running_stats.second_moment)
									+ (((job->burst_time)*
										(job->burst_time))/(running_stats.n +1));
	running_stats.n +=1;

	TRACE("Waiting time updated: %f\n Running Time updated: %f\n Turnaround time updated: %f\n",
			waiting_stats.first_moment,
			running_stats.first_moment,
			turnaround_stats.first_moment);


	print_output_stat(0, job);

	free(job);
}

static void job_scheduled(JOB *current_job)
{
	TRACE("Schedule %d at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
	message_sink(current_job->pid, scheduler.clock_scheduler.ticks, FALSE);
	return;
}

static void job_preempted(JOB *current_job)
{
	TRACE("%d Preempted at %d\n", current_job->pid,  scheduler.clock_scheduler.ticks);
	message_sink(current_job->pid, scheduler.clock_scheduler.ticks, TRUE);
	return;	
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

			job_scheduled(current_job);
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
			job_completed(current_job);

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
				job_scheduled(current_job);
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

			job_scheduled(current_job);
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
			job_completed(current_job);

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
				job_scheduled(current_job);
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

			job_scheduled(current_job);
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
			job_completed(current_job);

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
				job_scheduled(current_job);
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
				job_preempted(current_job);

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
				job_scheduled(current_job);
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

			job_scheduled(current_job);

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
			job_completed(current_job);

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
				job_scheduled(current_job);

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
				job_preempted(current_job);

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
					job_scheduled(current_job);
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

			job_scheduled(current_job);
		}
	}
	else
	{
		/* There is a job executing. Has it completed execution? */
		current_job->run_time++;
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			job_completed(current_job);

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
				job_scheduled(current_job);
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
				job_preempted(current_job);

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
				job_scheduled(current_job);
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

			job_scheduled(current_job);

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
			job_completed(current_job);

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
				job_scheduled(current_job);

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
				job_preempted(current_job);

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
					job_scheduled(current_job);
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
			job_preempted(current_job);

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
			job_scheduled(current_job);

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

			job_scheduled(current_job);

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
			job_completed(current_job);

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
				
				job_scheduled(current_job);
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
				job_preempted(current_job);

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
					job_scheduled(current_job);
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
			job_preempted(current_job);
			
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
			job_scheduled(current_job);
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
	JOB *job=NULL;
	JOB *current_job = NULL;

	TIMESTAMP *ts = NULL;

	/* All jobs initially have a vruntime of 0. So */
	if((*incoming_job_queue).next != (*incoming_job_queue).self)
	{
		for(job = (JOB *)NEXT_IN_LIST(*incoming_job_queue);
			job != NULL;
			job = (JOB *)NEXT_IN_LIST(*incoming_job_queue))
		{
			REMOVE_FROM_LIST(job->node);
			INSERT_BEFORE(job->node, scheduler.job_scheduler.cfs.comn.pending_jobs_queue);
			TRACE("%5d |%10d |%10d |%10d |%10d |\n",job->pid, job->arrival_time, 
						job->burst_time, job->priority, job->is_foreground);
			scheduler.job_scheduler.cfs.current_load +=1;
			if(scheduler.job_scheduler.cfs.min_prio==0)
			{
				scheduler.job_scheduler.cfs.min_prio = job->priority;	
			}
			else
			{
				scheduler.job_scheduler.cfs.min_prio = (job->priority<scheduler.job_scheduler.cfs.min_prio)?
															job->priority:scheduler.job_scheduler.cfs.min_prio;	
			}
		}
	}

	/* Is there a job finishing in this time instant? */
	current_job = scheduler.job_scheduler.cfs.comn.job_in_service;
	if(current_job==NULL)
	{
		/* IDLE CPU. Find next job if available */
		/* Pop next job from queue and update its service in time. */
		current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.cfs.comn.pending_jobs_queue);
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

			job_scheduled(current_job);

			/* Update next switch time to either current_time+slice*/
			scheduler.job_scheduler.cfs.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.cfs.comn.time_slice;
		}
	}
	else
	{
		current_job->run_time++;
		/* Has the current job finished executing? */
		if(current_job->run_time==current_job->burst_time)
		{
			/* Pass it to sink module */
			job_completed(current_job);
			scheduler.job_scheduler.cfs.current_load--;

			/* Schedule next job.*/
			/* Pop next job from queue and update its service in time. */
			current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.cfs.comn.pending_jobs_queue);
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
				job_scheduled(current_job);

				/* Update next switch time to either current_time+slice*/
				scheduler.job_scheduler.cfs.comn.next_switch = 
										scheduler.clock_scheduler.ticks + 
										scheduler.job_scheduler.cfs.comn.time_slice;
			}
		}
		/* Else, have we exceeded out time slice? */
		else if(scheduler.job_scheduler.cfs.comn.next_switch 
										== scheduler.clock_scheduler.ticks)
		{
			/* TODO */
			/* Calculate new vruntime and see if it is still less than the head of queue */
			/* Swap it out. Move it to the end of pending queue if there is another process which is waiting */
			if(NEXT_IN_LIST(scheduler.job_scheduler.cfs.comn.pending_jobs_queue)!=NULL)
			{
				INSERT_BEFORE(current_job->node, 
							scheduler.job_scheduler.cfs.comn.pending_jobs_queue);

				TRACE("Swap %d out at %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
				job_preempted(current_job);

				/* Pop next job from queue and update its service in time. */
				current_job = (JOB *)NEXT_IN_LIST(scheduler.job_scheduler.cfs.comn.pending_jobs_queue);
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
					job_scheduled(current_job);
				}
				/* Update next switch time to either current_time+slice*/
				scheduler.job_scheduler.cfs.comn.next_switch = 
									scheduler.clock_scheduler.ticks + 
									scheduler.job_scheduler.cfs.comn.time_slice;
			}
		}
		/* Else continue doing what we were doing. */
	}

	/* By now, either current job is still executing, or has been swapped out or has finished executing */
	scheduler.job_scheduler.cfs.comn.job_in_service = current_job;
	if(scheduler.job_scheduler.cfs.comn.job_in_service != NULL)
	{
		//TRACE("Process %d running at instant %d\n", current_job->pid, scheduler.clock_scheduler.ticks);
	}

	return;
}