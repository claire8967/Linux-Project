#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

struct cs
{	

	unsigned int hw_count;
};


asmlinkage int sys_stop(unsigned int result)
{	
    int number = current -> hw_count;
    int *temp = &number;
    copy_to_user(result,temp,sizeof(int));
    return 0;
}