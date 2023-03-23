#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uacess.h>



asmlinkage int sys_start(unsigned int *result)
{	
    current -> hw_count = 0;
    int number = current -> hw_count;
    int *temp = &number;
    copy_to_user(result,temp,sizeof(int));
    return 0;
}