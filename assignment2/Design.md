## Design Option (A)

Here is one design that I can think of.

1. A block that reads jobs from a file and feeds them to a scheduler. Each job has a structural representation where its arrival time, burst time, priority, finish time and other relevant attributes are kept. Let's call this block the `dispatcher`

2. The scheduler runs the jobs to completion and passes each completed job to a data collection block. The scheduler itself has two major components - one is generic and other is problem specific. Let's call them the `clock scheduler` and the `job scheduler`. The `clock scheduler` simulates the passage of time (it is a discrete event simulation) and hence maintains the simulation's system clock. The `job scheduler` maintains a list of jobs that are currently in service and tracks their lifetime. 

At each tick, the `clock scheduler` freezes the complete program, asks the job `dispatcher` for jobs that might have arrived during the last interval. For example, suppose the time `t` starts at 0. So, the `clock scheduler` asks the `dispatcher` at some time `T` if there are any jobs that have arrival time as `t=T`? If there are any, the `dispatcher` must pass them to the `clock scheduler` in some representation (a linked list?). Then, the `clock scheduler` passes them to the `job scheduler`. The `job scheduler` looks into its `job queue` and checks if any of the ones that were being serviced before should be completing now (that is, their burst time is finished). Or, based on the policy being implemented, may re-schedule the old jobs and also schedule the new jobs that were passed to it. At each re-scheduling, the `job scheduler` adds an entry in the `trigger times` entry of the job (so that we can later construct a gantt chart out of it).

3. Finally, when the job completes its time in the `job scheduler`, the job scheduler marks its finish time and passes it to the `data collection` module where statistics of mean and other things are generated. Also, gantt chart is generated there.

So, in this design, only the `job scheduler` is the one which is specific to each problem, and rest of the things are same for all.