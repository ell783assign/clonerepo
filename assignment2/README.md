<center>

## **<u><font color="#006600">Assignment 2</font></u>**

</center>

<center>

## <font color="#CC0000">Topic: CPU Scheduling  

</font><font color="#000000">Due on or before:</font> <font color="#3333FF">18 March, 2017 (Saturday)</font>

</center>

<center>

## <font color="#000000">Maximum Marks: 7</font>

</center>

<center>

* * *

</center>

Simulate the following CPU scheduling algorithms:

*   FIFO
*   SJF
*   Round-Robin
*   Priority-based Scheduling
*   Multi-level Queues
*   Multi-level Feedback Queues
*   Linux's Scheduler

* * *

## <font color="#3333FF">Take-Home Message</font>

This assignment gives an important insight into a typical way a CPU schedulign algorithm is designed before implementing it in a running system: simulation. Simulation is a well-studied science: see Narsingh Deo's famous book on scheduling, for instance. Simulation is not `the last refuge of scoundrels' so to say, but is often the only way out in cases which cannot be handled by analytic means alone. There are many results which ware known empirically: they have come through numerous simulation runs. Assume a random number of jobs, each with a CPU burst time given by a random amount. (You can take these from a random number generator, and experiment with different distributions). For each case, please compute statistics (mean, standard deviation for parameters of interest, such as the waiting time, and others). In each case, you can add more sophistication e.g., have a time-varying priority scheme with ageing, or a multi-level queue with flexibility like Linux's earlier scheduler. More the sophistication, better will be your chances of getting higher marks!

* * *

## Developers' section   
**fcfs.c** - First-come-first-served implementation
* * *
