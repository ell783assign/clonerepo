#ifndef __OUTPUT_H__
#define __OUTPUT_H__
/**
 * @brief Print opening and closing decorator of gantt chart.
 */
 
 void gantt_headers();
/*
 * @brief   message_sink(): 
 *          Prints the Gantt Chart.
 *          Called every time a job is scheduled or completes.
 * 
 * @params  int pid:
 *          The process id is passed here.
 * 
 *          int end_time:
 *          The time instant this function was called.
 * 
 *          int action: 
 *          Takes value either `0` or `1`
 *          Action is either taking a process off ready queue - `0`
 *          Or a process being completed - `1`
 * 
 * @return  None            
 */
void message_sink(int , int , int );

/*
 * @brief print_output_stat():
 *        Prints average waiting time and average turnaround time. Called twice. 
 *        First usage-->print_output_stat(dispatcher.num_jobs, NULL)
 *        in order to declare the array size inside the function.
 *        Second time-->print_output_stat(0, job). 
 *        The job information is then added onto the previous info to calculate avg. waiting
 *        time and avg.turnaround time.
 * 
 * @params  int num_jobs:
 *          Number of total jobs. This is also the number of times this function
 *          shall be called.
 * 
 *          void *job:
 *          The job that has just completed.         
 */
void print_output_stat(int , JOB *);


typedef struct
{
    int sum_waiting_time;
    int sum_turnaround_time;
}Process;

typedef struct stats
{
	float n;
	float first_moment;
	float second_moment;
	float variance;
}STATS;

#ifdef INCLUDE_GLOBALS

int times_called_global = 0;
int init_time_global = 0;
int iteration_global = 0;
int num_jobs_global = 0;


Process p;

STATS waiting_stats = {0};
STATS turnaround_stats = {0};
STATS running_stats = {0};

#else

extern int times_called_global;
extern int init_time_global;
extern int iteration_global;
extern int num_jobs_global;
extern Process p;
extern STATS waiting_stats;
extern STATS turnaround_stats;
extern STATS running_stats;
#endif

#endif
