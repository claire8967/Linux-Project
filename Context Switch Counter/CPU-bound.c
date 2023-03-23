#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <syscall.h>
#include <sched.h>

int main()
{
	int a;
	int s;
	s = 1; 
	float b = 0;

	unsigned long diff;
	struct timeval start;
	struct timeval end;
    
        int *temp1;
        int *temp2;
        int number1 = 0;
        int number2 = 0;
        temp1 = &number1;
        temp2 = &number2;
    
	syscall(317,temp1);
	printf("This process has encounter %u times context switches.\n",number1);
	gettimeofday(&start,NULL);

	while(s==1){
		b = b + 1;
		gettimeofday(&end,NULL);
		diff = 1000000 * (end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec;
		if(diff>120000000){
			s = 0;
		}
	}
	a = syscall(318,temp2;
	printf("During the past two minutes the process makes %d times process switches.\n",number2);
	return 0;
}