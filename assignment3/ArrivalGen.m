%% Generate a job schedule of n jobs from an exponential distribution
%% First value in any row is the arrival time. The second is burst time.
lambda=1;
mu=1;
n=10000;

fd = fopen('jobs.txt', 'w+');
jobs = zeros(n,2);
jobs(1,1) = (-(1/lambda)*log(rand(1)))*1000; %in ms
jobs(1,2) = (-(1/lambda)*log(rand(1)))*1000; %in ms
for ii=2:n
  jobs(ii,1)= jobs(ii-1,1)+((-(1/lambda)*log(rand(1)))*1000); %Arrival time
  jobs(ii,2)= (-(1/mu)*log(rand(1)))*1000; %Service time
end

fprintf(fd, '%5.0f\n', n);
for ii=1:n
   fprintf(fd, '%.0f %5.0f %5.0f\n', ii, jobs(ii,1), jobs(ii,2));
end

fclose(fd);