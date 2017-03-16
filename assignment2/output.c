#include <include.h>

void gantt_headers()
{
    printf("\n********************DISPLAYING GANTT CHART***********************\n");
    return;
}

void message_sink(int pid, int end_time, int action)
{
    if(times_called_global<=num_jobs_global && action)
    {
        printf("| P%d(%d-%d) |", pid, init_time_global, end_time);
    
        /*New init_time is last end_time*/
        init_time_global = end_time;
    }
    else
    {
        /* if action is 0, end_time has data about start time. */
        init_time_global = end_time;   
    }
    return;
}

void print_output_stat(int num_jobs, JOB *job_completed)
{
    /*Temporary variables*/
    int turnaround_time;
    int waiting_time;
    JOB *job = job_completed;
    
    /*First usage*/
    if(num_jobs!=0)
    {
        num_jobs_global = num_jobs;
        p.sum_waiting_time = 0;
        p.sum_turnaround_time = 0;
    }
    
    /*Subsequent usage*/
    else
    {
        iteration_global++;
        /* As long as jobs remain to be called */
        if(iteration_global <= num_jobs_global)
        {
            turnaround_time = job->finish_time - job->arrival_time;
            waiting_time = turnaround_time - job->burst_time;
            p.sum_waiting_time += waiting_time;
            p.sum_turnaround_time += turnaround_time;
        }
        
        /* No more jobs shall come now. Print final result */
        if(iteration_global==num_jobs_global)
        {
            turnaround_time = job->finish_time - job->start_time;
            waiting_time = turnaround_time - job->burst_time;
            p.sum_waiting_time += waiting_time;
            p.sum_turnaround_time += turnaround_time;
            printf("\n*****************************************************************\n");
            printf("\n\nAverage waiting time: %.5f\nAverage turnaround time: %.5f\n\n", 1.0*p.sum_waiting_time/num_jobs_global, 1.0*p.sum_turnaround_time/num_jobs_global);
        }
        return;
    }
    return;
}


