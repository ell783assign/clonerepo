##Problem Statement

There is a service counter which has a limited waiting queue outside it. It works as follows:

* The counter remains open till the waiting queue is not empty
* If the queue is already full, the new customer simply leaves
* If the queue becomes empty, the outlet doors will be closed (service personel sleep)
* Whenever a customer arrives at the closed outlet, he/she needs to wake the person at the counter with a wake-up call


Implement the above-described problem using semaphores or mutuexes along with threads. 

Also show how it works, if there are 2 service personel, and a single queue. Try to simulate all possible events that can take place, in the above scenario.

###Take Home Message

The assignment aims to provide an opportunity to design a solution to a realistic synchronisation problem, and implement it with semaphores and mutexes. The experimentation is to illustrate the correctness of the design.


###Design Proposal

__System Parameters__:

1. Number of service counters (1/2)
2. Waiting Queue capacity.
3. Number of threads = Number of service counters + Producer thread (2/3)

__Design Assumptions__:

1. Time is quantized to milliseconds (or less).
2. So far, we don't know what the test cases are going to look like. Assume that a service requests arrive following a Poisson distribution (exponential inter-arrival time).


__Design Discussions__:

Let's discuss the simple scenario where there is just one customer service counter. It involves 2 threads - one for job production and the other for job consumption. 

We define a global _clock_ variable as a monotonically incrementing variable that keeps track of time elapsed. This variable must not be changed by the consumer (service counter) thread. Also, before the value is incremented, the consumer thread must be given a chance to complete processing in the current time slot. Thus, it should be _protected by a mutex lock_ that alternates between the producer and consumer thread.

For example, consider how the simulation would run:

Let arrivals be like:

|Job		|  	Arrival		|   	Service Time	|
|---------------|-----------------------|-----------------------|
| 1		|	 0.3		|	2		|
|---------------|-----------------------|-----------------------|
| 2		|	 1.5		|	1		|
|---------------|-----------------------|-----------------------|
| 3		|        2.3		|	1		|
|---------------|-----------------------|-----------------------|

Queue capacity: 1


|Time|  Producer Thread				   | Consumer Thread   				 |
|----|---------------------------------------------|---------------------------------------------|
|t=1 | Check if new jobs arrived in interval (0,1] | Check if there is some job currently running|
|    | Occupancy=0; Capacity=1; Enqueue Job 1	   | No running job, Dequeue Job 1 and service	 |
|----|---------------------------------------------|---------------------------------------------|
|t=2 | Check if new jobs arrived in interval (1,2] | Check if there is some job currently running|
|    | Occupancy=0; Capacity=1; Enqueue Job 2	   | Job 1 running (till t=1+2)			 |
|----|---------------------------------------------|---------------------------------------------|
|t=3 | Check if new jobs arrived in interval (2,3] | Check if there is some job currently running|
|    | Occupancy=1; Capacity=1; Drop Job 3	   | Job 1 finished. Dequeue Job 2 and service	 |
|----|---------------------------------------------|---------------------------------------------|

Producer thread looks into the next jobs arrival time and goes to sleep until that period while the consumer thread grabs the head of line job and goes to sleep.
