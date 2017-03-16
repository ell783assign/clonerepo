#ifndef SCHEDULER_H_
#define SCHEDULER_H_

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

typedef struct reschedule_timing
{
	CLL node;
	int32_t ts;
}TIMESTAMP;


typedef struct job
{
	CLL node;
	uint32_t pid;
	uint32_t arrival_time;
	uint32_t burst_time;
	uint32_t priority;
	uint32_t is_foreground;

	int32_t start_time;
	int32_t run_time;
	int32_t finish_time;
	int32_t remaining_time; /* Added just so that we can sort array in SJF by it (for generic nature of insert sort algo)*/
	int32_t vruntime;
	CLL ts_root;
}JOB;

typedef enum 
{  
	FCFS=0,
	SJF_NP,
	RR,
	SJF_P,
	PRIORITY,
	MULTILEVEL_Q,
	MF_Q,
	CFS,

	__MAX_SCHEDULER_COUNT__
}JOB_SCHEDULER_TYPES;

typedef struct _dispatch
{
	int32_t num_jobs;

	CLL job_list_root;

	char *file_name;


	int32_t num_jobs_remaining;

}DISPATCH;

typedef struct _clock_scheduler
{
	uint32_t ticks;

	uint32_t signal_stop;

}CLOCK_SCHEDULER;

typedef void (Feed_Jobs)(CLL *job_root);
/**
 * This part is common to all schedulers.
 */
typedef struct job_scheduler_comn
{
	int32_t time_slice;
	int32_t next_switch;
	int32_t policy;

	JOB *job_in_service; /**< Only one job can be running at a time, in a single core single CPU system */
	CLL pending_jobs_queue;

	Feed_Jobs *feeder;
}JOB_SCHEDULER_COMN;

typedef struct fcfs
{
	JOB_SCHEDULER_COMN comn;
}FCFS_SCHED;

typedef struct sjf_p
{
	JOB_SCHEDULER_COMN comn;
}SJF_SCHED;

typedef struct rr
{
	JOB_SCHEDULER_COMN comn;
}RR_SCHED;

typedef struct sjf_np
{
	JOB_SCHEDULER_COMN comn;
}SJF_NP_SCHED;

typedef struct prio
{
	JOB_SCHEDULER_COMN comn;
}PRIO_SCHED;

typedef struct multilevel
{
	JOB_SCHEDULER_COMN comn;
	CLL foreground;
	CLL background;
}ML_SCHED;

typedef struct multilevel_feedback
{
	JOB_SCHEDULER_COMN comn;
}ML_FB_SCHED;

typedef struct cfs
{
	JOB_SCHEDULER_COMN comn;
	BST tree;
}CFS_SCHED;

typedef struct _job_scheduler
{
	int32_t current_scheduler;

	FCFS_SCHED fcfs;
	SJF_SCHED sjf;
	SJF_NP_SCHED sjf_np;
	RR_SCHED rr;
	PRIO_SCHED prio;
	ML_SCHED ml;
	ML_FB_SCHED ml_fb;
	CFS_SCHED cfs;
}JOB_SCHEDULER;

typedef struct _scheduler
{
	CLOCK_SCHEDULER clock_scheduler;
	JOB_SCHEDULER job_scheduler;
}SCHEDULER;


#ifdef INCLUDE_GLOBALS
struct textual_names 
{
	uint32_t index;
	char name[50];
} menu[__MAX_SCHEDULER_COUNT__] = 
{
	{0, "First Come First Served Scheduler"},
	{1, "Shortest Job First Scheduler (Non-Preemptive)"},
	{2, "Round Robin Scheduler"},
	{3, "Shortest Job First Scheduler (Preemptive)"},
	{4, "Priority Based Scheduler"},
	{5, "Multi-Level Queues Scheduler"},
	{6, "Multi-Level Feedback Queues Scheduler"},
	{7, "Completely Fair Scheduler"},
};

DISPATCH dispatcher;
SCHEDULER scheduler;
#else
extern struct textual_names menu[__MAX_SCHEDULER_COUNT__];
extern DISPATCH dispatcher;
extern SCHEDULER scheduler;;
#endif

void feed_fcfs(CLL *);
void feed_sjf_np(CLL *);
void feed_sjf(CLL *);
void feed_rr(CLL *);
void feed_prio(CLL *);
void feed_ml(CLL *);
void feed_mfq(CLL *);
void feed_cfs(CLL *);

#endif