#ifndef SCHEDULER_H_
#define SCHEDULER_H_

typedef struct circular_linked_list
{
	void *self;
	struct circular_linked_list *next;
	struct circular_linked_list *prev;
}CLL;

#define INIT_CLL_ROOT(LIST)					\
	LIST.self = NULL;						\
	LIST.prev = NULL;						\
	LIST.next = NULL;

#define INIT_CLL_NODE(NODE,SELF)			\
	NODE->self = SELF;						\
	NODE->next = NULL;						\
	NODE->prev = NULL;		

//Read as Insert A after B
#define INSERT_AFTER(A,B)					\
	A->next = B->next;						\
	A->next.prev = A;						\
	B->next = A;							\
	A->prev = B;	

//Read as Insert A before B
#define INSERT_BEFORE(A,B)					\
	A->next = B;							\
	B->prev.next = A;						\
	A->prev = B->prev;						\
	B->prev = A;

#define REMOVE_FROM_LIST(A)					\
	A->next.prev = A->prev;					\
	A->prev.next = A->next;					\
	A->prev = NULL;							\
	A->next = NULL;			


typedef struct reschedule_timing
{
	struct reschedule_timing *next;
	int32_t ts;
}TIMESTAMP;


typedef struct job
{
	CLL node;
	uint32_t pid;
	uint32_t arrival_time;
	uint32_t burst_time;
	uint32_t priority;
	uint32_t is_background;

	int32_t finish_time;
	TIMESTAMP ts_root;
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
}JOB_SCHEDULER;

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
#else
extern struct textual_names menu[__MAX_SCHEDULER_COUNT__];
#endif



#endif