Context Switch 的底層機制及實驗
===
###### tags: `Linux`
    
## Q1
Write a new system call int get_CPU_number() so that a process can use it to get the number of the CPU that executes it.
### Kernel space code
#### System Call 316: get_CPU_number()

```clike=
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <asm/thread_info.h>
asmlinkage long sys_my_cpu(void)
{	
	//int cpu;
	//cpu = sched_getcpu();
	struct thread_info * info = current_thread_info();
	printk("number of cpu %u\n",info->cpu);
	return 0;
}
```
### Userspace code
用系統的 getcpu() 驗證結果

```clike=
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>
#include <sched.h>

static inline int getcpu(){
	int cpu,status;
	status = syscall(SYS_getcpu,&cpu,NULL,NULL);
	return (status == -1) ? status : cpu;
}


int main(){
	printf("getpid %d\n",getpid());
	syscall(317);
	printf("sched_getcpu = %u\n",getcpu());
}
```

![](https://i.imgur.com/6zOQ4np.jpg)


## Q2
Write a new system call start_to_count_number_of_process_switches() so that a process can use it to begin to count the number of process switches the process makes. 

Besides, write another new system call int stop_to_count_number_of_process_switches() so that a process can use it to stop to count the number of process switches the process makes and return the number of process switches the process makes.
### Kernel space code

```
cd /usr/src/linux-3.10.104/include/linux
vim sched.h
```

```clike=
struct task_struct {
    .
    .
    .
    .
    unsigned int hw_count;
}
```


```
cd /usr/src/linux-3.10.104/arch/x86/kernel
vim process_64.c
```

```clike=
int copy_thread(unsigned long clone_flags, unsigned long sp, unsigned long arg, struct task_struct *p){
    p-> hw_count = 0;
    .
    .
    .
    .
}


__switch_to(struct task_struct *prev_p, struct task_struct *next_p)
{
    prev_p-> hw_count = prev_p-> hw_count + 1;
    .
    .
    .
    .
}
```

#### System call 317: start_to_count_number_of_process_switches()

```clike=
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
```

#### System call 318: stop_to_count_number_of_process_switches()

```clike=
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
```

### Userspace code
使用  gettimeofday() 來確認程式執行時間，若執行時間大於兩分鐘，則終止程式。
#### CPU-bound program
```c
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
```
![](https://i.imgur.com/eWr6lqP.png)


運用系統內部的 nvcsw 和 nivcsw 驗證結果，可以發現整支程式的 context switch 次數與我們計算出的 context switch 次數相差不遠
![](https://i.imgur.com/pWNsJFl.png)




#### I/O-bound program

```c
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
        printf("b is %d: \n",b);
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
```
![](https://i.imgur.com/RnYSa6s.png)

運用系統內部的 nvcsw 和 nivcsw 驗證結果，可以發現整支程式的 context switch 次數與我們計算出的 context switch 次數相差不遠
![](https://i.imgur.com/jqXUkec.png)




       
## 補充資料


1. current->nivcsw : 非自願放棄 cpu 
   current->nvcsw : 自願放棄 cpu
   (用這兩個 register 的值可以大略判斷我們寫的 system call 的正確性)
   
2. 為何在 task_struct 新增變數時，不可新增在中間，必須新增在 task_struct 的最後 ?
    在 linux 中，有些指令是直接透過 offset 去 access task_struct 中的某個變數，也就是說如果我們在 task_struct 的中間新增變數打亂原本變數的排列方式，會影響到每個變數的 offset，造成某些指令無法正確執行。
    
3. 在 linux kernel 中，一個 process 進行 context switch 的流程為何 ?
    [Schedule()](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L3052)  -> [__schedule()](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L2955)  -> [context_swtich()](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L1984)  -> [switch_to()](https://elixir.bootlin.com/linux/v3.10.104/source/arch/x86/include/asm/switch_to.h#L31)
     + Kernel 一開始會呼叫 Schedule() 函數去決定 cpu 接下來要執行哪個 process，接著呼叫 context_switch() 函數去執行 context switch，而 switch_to() 函數則是負責切換 register 和 kernel mode stack ，也就是負責保留原本 process 的 register 值，並恢復欲切換的 process 的 register 值，然後才能執行新的 process。
     
     + 在 context_switch() 函數裡的 rq，指的是每一個 cpu 自己都有一個 run_queue，而這個 rq 會指向要發生 context switch 的 cpu 的 run_queue，prev 是指要交出 cpu 控制權的 process，next 是指得到 cpu 控制權的 process。


4. 什麼是 CFS scheduler ?
    CFS 就是要使每個 process 都盡可能公平的獲得執行時間，因此每次都選擇過去執行時間最少的，也就是在真實的硬體上實現公平的多任務調度。
    
    實際的方法就是 CFS 會維護每一個 task 的 virtual runtime。 Task 的 virtual runtime 愈小，表示其被允許使用的 CPU 時間愈短，也間接代表此 task 對 CPU 的需要更高，因此更需要被排程。CFS 同時也思考到了進入 sleep 狀態的任務(例如等待 I/O)之公平性，確保當他們醒來時可以得到足夠的 CPU 額度。
    
    另外，CFS 的 runqueue 的資料結構其實是藉由一個紅黑樹(RB-tree) 來維護的。
5. schedule 的流程為何 ?
    -> 完成必要檢查，設置 process 的狀態，處理 process 所在的 run queue
    -> 調用 __schedule() 中的 pick_next_task 函數選擇要將 cpu 控制權給哪個 process
    -> 如果目前 cpu 上所有的 process 都是 CFS 的普通 process，則直接使用 CFS，如果沒有 process 則調度 idle process
    -> 否則就從 priority 最高的 sched_class_highest(目前是 stop_sched_class)開始一次遍歷所有 scheduler 的 pick_next_task 函數，選擇執行最優的 process
    -> context_switch() 完成 process 的 context switch
    -> 調用 switch_mm()
    -> 調用 switch_to()

6. sched_class 裡有四種 class，他們的優先級排列順序是:
    stop_sched_class > rt_sched_class > fair_sched_class > idle_sched_class
    ```c
    extern const struct sched_class stop_sched_class;
    extern const struct sched_class rt_sched_class;
    extern const struct sched_class fair_sched_class;
    extern const struct sched_class idle_sched_class;
    ```
    | sched_class | description |strategy |
    |-------------|-------------|---------|
    |stop_sched_class|highest priority|None|
    |rt_sched_class|real time scheduler|SCHED_FIFO、SCHED_RR|
    |fair_sched_class|CFS scheduler|SCHED_NORMAL、SCHED_BATCH|
    |idle_sched_class|idle task|SCHED_IDLE|

## 補充
1. task_struct 中的 randomized struct field 是會打亂所有 task_struct 的排序，以防止被攻擊
2. linux OS 中會有跟 cpu 相同數量的 run queue
3. switch_to 中再往下會 call  __switch_to_asm() 去交換 register 內的值 
    ```clike=
    SYM_CODE_START(__switch_to_asm)
	/*
	 * Save callee-saved registers
	 * This must match the order in struct inactive_task_frame
	 */
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	/*
	 * Flags are saved to prevent AC leakage. This could go
	 * away if objtool would have 32bit support to verify
	 * the STAC/CLAC correctness.
	 */
	pushfl

	/* switch stack */
	movl	%esp, TASK_threadsp(%eax)
	movl	TASK_threadsp(%edx), %esp

    #ifdef CONFIG_STACKPROTECTOR
	movl	TASK_stack_canary(%edx), %ebx
	movl	%ebx, PER_CPU_VAR(__stack_chk_guard)
    #endif

	/*
	 * When switching from a shallower to a deeper call stack
	 * the RSB may either underflow or use entries populated
	 * with userspace addresses. On CPUs where those concerns
	 * exist, overwrite the RSB with entries which capture
	 * speculative execution to prevent attack.
	 */
	FILL_RETURN_BUFFER %ebx, RSB_CLEAR_LOOPS, X86_FEATURE_RSB_CTXSW

	/* Restore flags or the incoming task to restore AC state. */
	popfl
	/* restore callee-saved registers */
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp

	jmp	__switch_to
    SYM_CODE_END(__switch_to_asm)   
    ```
4. process 是怎麼從 wait queue 被喚醒至 run queue 的?
    一般來說所有的 process 在離開的時候都會呼叫 default_wake_function 函數或 autoremove_wake_function 函數，而他們兩個函數都會呼叫 try_to_wake_up 函數，try_to_wake_up 的流程如下:
    [try_to_wake_up()](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L1485) -> [ttwu_queue()](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L1485) -> [ttwu_do_active](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L1356) -> [ttwu_do_wakeup](https://elixir.bootlin.com/linux/v3.10.104/source/kernel/sched/core.c#L1332) -> TASK_RUNNING
    
## Reference
https://www.zendei.com/article/60847.html
https://blog.csdn.net/sucjhwaxp/article/details/106150494
https://blog.csdn.net/u012489236/article/details/122384915
https://hackmd.io/@RinHizakura/B18MhT00t