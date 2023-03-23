製作將 virtual address 轉成 physical address 的 System call
===
###### tags: `Linux`

---


## 實驗假設

**1. Copy-on-write(COW)**：我們可以假設任何資源在尚未更動前皆共享已被創建出來的資源，比如`clib`，而一個新被創建出來的process在被更動(write)前應該也是共享同樣的page frame。

**2. Library：** 因為Library是不會被更動到的外部引用資源，所以應該是所有要引用的processes共用同一份。


## 實驗環境
VirtualBox : Ubuntu 16.04.6 64bit
Kernel Version : 3.10.105

## 實驗目標
+ Write a program using the system call you wrote in Project 1 to check how memory areas are shared by two processes that execute this program simultaneously.
+ The memory areas include code segments, data segments, BSS segments, heap segments, libraries, stack segments.



## 實驗結果／驗證

#### 作法一
運用上次 Project1 的兩個 system call，分別執行兩支程式 process1 和 process2，並且在程式最後加上 sleep(100)，以觀察兩個 Process 執行時的記憶體狀態，圖上的最後一行為該 segment 的起始 physical address。

直覺上來說，會認為兩個 Process 間只有 kernel address space 會共享，剩下的 user address space 不會有 shared memory，然而實驗在部分segment的結果卻並非完全如我們所想。

![](https://i.imgur.com/gvxTuNu.jpg)

放大圖:
pid 2521
![](https://i.imgur.com/FheGWD7.jpg)
pid 2547
![](https://i.imgur.com/2kD6ViV.jpg)


由上圖的實驗可知，兩個 process 之間的
    1. data segment
    3. bss segment
    4. heap segment
    5. stack segment
    皆不會共享。
    
但我們可以發現，
1. code segment 有被共享
2. 在 library segment，library 的 code segment 是有被共享的 (黃色)，但 library 的 data segment 是沒有被共享的 (藍色)。
    發生這個現象的原因是，當兩個 process 要使用相同資料時，在一開始只會 loading 一份資料，並且被標記成 read-only 。當有 process 要寫入時則會對 kernel 觸發 page fault ，進而使得 page fault handler 處理 copy on write 的操作，而 library 的 code segment 並沒有被更改，所以兩個 process 的 physical address 是一樣的，反之，因為 library 的 data segment 有被更改，所以 physical address 不同。


#### 作法二
運用上次 Project1 的兩個 system call，執行一支程式 process1，並使用 fork() 創造出 child process，以觀察Parent process 和 Child process 執行時的記憶體狀態，圖上的最後一行為該 segment 的起始 physical address。

pid 2521
![](https://i.imgur.com/z5vh7z1.jpg)

pid 2520
![](https://i.imgur.com/czq5TTU.jpg)

1. 第二個作法做出的 data segment 的 start physical address 相同，但 end segment address 不同 
2. 我們可以發現，透過 fork()，創造出的 child process 的 virtual address 和 parent process 的 virtual address 完全相同 (映射到相同的記憶體位置)，但當我們修改 child process 的值時，copy_on_write 即會發生，使其映射到不同的記憶體位置。

另外，我們也可由作法一和作法二得知:
1. 在不同 process 中，不同的 virtual address 有可能對應到相同或者是不相同的 physical address.
2. 在不同 process 中，相同的 virtual address 有可能對應到相同或者是不相同的 physical address.


Shared Library： 以往系統採用static linking的方式，即library的object code也會在引用該library的executable file內，這會使得若有多個process要引用同一個library，便需要儲存多份一樣的library code造成空間上的浪費。而現今多數系統則有別於以往static linking的方式，採用**shared libraries**的機制，也就是讓program的executable file不包含library的object code，僅會提及(refer)他要用到libraries的名稱。直到執行階段dynamic linker才會去檢視executable file中提及到的library，讓該process能夠去訪問那個library的requested code。

實際上運作的方式是讓process的address space映射(mapping)到要使用的library file，大大節省了記憶體空間。


## 延伸補充
### 兩個 Process 如何透過共享記憶體溝通
在兩個 Process 之間建立一個 Shared Memory `shm`，`write.c` 負責寫入資料，`read.c` 用來讀取資料。最後利用 System Call 來觀察是否有共享記憶體。
```c=
// write.c 寫入資料
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <time.h>

#define SHM_SIZE 2048

int main() {
    int shm_id = shmget(13, SHM_SIZE, IPC_CREAT | 0666);

    if (shm_id != -1) {
	void* shm = shmat(shm_id, NULL, 0);

	if (shm != (void*)-1) {
	    char str[] = "This is a string";
	    unsigned long physical = 0;

	    memcpy(shm, str, strlen(str) + 1);
	    printf("[INFO] Write data \"%s\" to a shared memory segment\n", str);
	    printf("[INFO] Shared Memory Virtual  Address = %p\n", shm);
            // System Call 351 為虛擬位址轉實體位址
	    syscall(351, shm, (void*) &physical);
	    printf("[INFO] Shared Memory Physical Address = 0x%5lx\n", physical);
	    shmdt(shm);
	} else {
	    perror("shmat:");
	}
    } else {
	perror("shmget:");
    }

    sleep(10000); // 為了讓兩個 Process 同時都在執行
    return 0;
}
```
```c=
// read.c 讀取資料
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <time.h>

#define SHM_SIZE 2048

int main() {
    int shm_id = shmget(13, SHM_SIZE, IPC_CREAT | 0666);
    
    if (shm_id != -1) {
	void* shm = shmat(shm_id, NULL, 0);
	if (shm != (void*)-1) {
	    char str[50] = { 0 };
	    unsigned long physical = 0;

	    printf("[INFO] Shared Memory Content = \"%s\"\n", (char *) shm);
	    printf("[INFO] Shared Memory Virtual  Address = %p\n", shm);
            // System Call 351 為虛擬位址轉實體位址
	    syscall(351, shm, (void*) &physical);
	    printf("[INFO] Shared Memory Physical Address = 0x%5lx\n", physical);
	    shmdt(shm);
        } else {
	    perror("shmat:");
	}
    } else {
        perror("shmget:");
    }

    sleep(10000); // 為了讓兩個 Process 同時都在執行
    return 0;
}
```
![](https://i.imgur.com/y5XZEgt.png)

開兩個終端機同時執行 `write.c` 和 `read.c`， `write.c` 會將字串寫入共享記憶體 `shm`，而 `read.c` 會將共享記憶體的內容讀取出來，由結果可知兩個 Process 間可以透過共享記憶體互相傳輸資料。另外，兩個 Process 印出的 `shm` 實體位址皆相同可以證實兩個 Process 間有共享記憶體。
#### [shmget()](https://elixir.bootlin.com/linux/v3.10.105/source/ipc/shm.c#L610) 建立共享記憶體
利用 shmget() 可以建立一個共享記憶體區域，以下為 Trace Code 過程 : 
```
shmget()
ipcget()
ipcget_new() : 建立一個 IPC (Interprocess Communication) Object
newseg() : 建立一個 Shared Memory Segment
ipc_rcu_alloc() : 配置 IPC Object 和 RCU Header 的記憶體空間
ipc_alloc() : 配置 IPC Object 記憶體空間
vmalloc() / kmalloc() : 實體記憶體配置
```
## 參考資料
https://stackoverflow.com/questions/53040242/how-can-two-processes-share-the-same-shared-library

---
## Userspace CODE
```c
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

pthread_t ntid;
__thread int thread_var = 10;
int global_var_data = 3;
int global_var_bss;

struct map{
	unsigned long start; //segment start address
	unsigned long end;   //segment end address
	char flags[5]; //read, write, exection flages
	char name[40]; //maping file name
	unsigned long physical;  //physical addresss
	unsigned long physical_end;
}my_map[40];

int get_vmmap(){
	int len = 0;
	while(syscall(314, len, (void*)&my_map[len])){
		my_map[len].physical = 0;
		//my_map[len].physical_end = 0;
		len++;
	}
	return len;
}

void find_thread_segmentation(int len, int *vir, char *count){
	int i;
	for(i = 0; i < len; i++){
		if((unsigned long)vir >= my_map[i].start && (unsigned long)vir <= my_map[i].end){
			//syscall(352, my_map[i].start, (void*)&my_map[i].physical);
			syscall(315, vir, (void*)&my_map[i].physical);
			//syscall(315, vir, (void*)&my_map[i].physical_end);
			strcat(count, " thread stack");
			strcpy(my_map[i].name, count);
		}
	}
	
}

void lib_physical(int len, char *lib){
	int i;
	for(i = 0; i < len; i++){
		if(strcmp(my_map[i].name, lib) == 0){
			syscall(315, my_map[i].start, (void*)&my_map[i].physical);
			//syscall(315, my_map[i].end, (void*)&my_map[i].physical_end);
		}
	}

}

void code_physical(int len){
	int i;
	for(i = 0; i < len; i++){
		if(strcmp(my_map[i].name, "code") == 0){
			syscall(315, my_map[i].start, (void*)&my_map[i].physical);
			//syscall(315, my_map[i].end, (void*)&my_map[i].physical_end);
		}
	}
}
void data_physical(int len){
	int i;
	for(i = 0; i < len; i++){
		if(strcmp(my_map[i].name, "data") == 0){
			syscall(315, my_map[i].start, (void*)&my_map[i].physical);
			//syscall(315, my_map[i].end, (void*)&my_map[i].physical_end);
		}
	}
}

void stack_physical(int len, int *p){
	int i;
	for(i = 0; i < len; i++){
		if(strcmp(my_map[i].name, "main stack") == 0){
			syscall(315, p, (void*)&my_map[i].physical);
			break;
		}
	}
}
void heap_physical(int len){
	int i;
	for(i = 0; i < len; i++){
		if(strcmp(my_map[i].name, "main heap") == 0){
			syscall(315, my_map[i].start, (void*)&my_map[i].physical);
			//syscall(315, my_map[i].end, (void*)&my_map[i].physical_end);
			break;
		}
	}
}

void print_vmmap(int len){
	int i;
	for(i = 0; i<len; i++){
		printf("0x%012lx ", my_map[i].start);
		printf("0x%012lx ", my_map[i].end);
		printf("%s  ", my_map[i].flags);
		printf("%-20s  ", my_map[i].name);
		printf("%5lx ", my_map[i].physical);
		//printf("%5lx ", my_map[i].physical_end);
		printf("\n");
	}
}

void *child_thread_one(void * arg){
	sleep(1);
	char count[20] = "one";
	int local_var = 1;
	unsigned long pfn_local_var, pfn_thread_var;

	//thread_var + 1
	thread_var = thread_var + local_var;

	//virtual address -> physical address
	syscall(315, &local_var, (void*)&pfn_local_var);
	syscall(315, &thread_var, (void*)&pfn_thread_var);

	find_thread_segmentation(global_var_data, &local_var, count);

	return NULL;
}


int main(void){
	
	pid_t pid;
	int status;
	pid = fork();
	

	if(pid==0){
		printf("child getpid %d",getpid());
		int err, local_var = 1;
		int *heap_var;
		heap_var = (int*)malloc(sizeof(int));
		*heap_var = 300;
		unsigned long pfn_local_var, pfn_global_var_data, pfn_global_var_bss, pfn_const_var,pfn_heap_var;
		err = pthread_create(&ntid, NULL, child_thread_one, NULL);
		if(err != 0) printf("child_thread_one error \n");

		global_var_data = get_vmmap();
	
	
		syscall(315, &local_var, (void*)&pfn_local_var);
		syscall(315, &global_var_data, (void*)&pfn_global_var_data);
		syscall(315, &global_var_bss, (void*)&pfn_global_var_bss);
		syscall(315, &heap_var, (void*)&pfn_heap_var);	
	
		printf("\n");
		printf("global_var_data = %d, address = %p, pfn = 0x%lx \n", global_var_data, &global_var_data, pfn_global_var_data);
		printf("\n");
		printf("global_var_bss = %d, address = %p, pfn = 0x%lx \n",global_var_bss, &global_var_bss,pfn_global_var_bss);
		printf("\n");
		printf("local_var = %d, address = %p, pfn = 0x%lx \n", local_var, &local_var, pfn_local_var);
		printf("\n");
		printf("heap_var = %d, address = %p, pfn = 0x%lx \n", *heap_var, heap_var, pfn_heap_var);
		printf("\n");
		sleep(4);

		lib_physical(global_var_data, "libpthread-2.19.so");
		lib_physical(global_var_data, "process1");
		lib_physical(global_var_data, "libc-2.19.so");
		lib_physical(global_var_data, "ld-2.19.so");
		stack_physical(global_var_data, &local_var);
		heap_physical(global_var_data);
		code_physical(global_var_data);
		data_physical(global_var_data);
		print_vmmap(global_var_data);
		sleep(100);
		
	}else{	
		sleep(50);
		printf("parent getpid %d",getpid());
		int err, local_var = 0;
		int *heap_var;
		heap_var = (int*)malloc(sizeof(int));
		*heap_var = 200;
		unsigned long pfn_local_var, pfn_global_var_data, pfn_global_var_bss, pfn_const_var,pfn_heap_var;
		err = pthread_create(&ntid, NULL, child_thread_one, NULL);
		if(err != 0) printf("child_thread_one error \n");

		global_var_data = get_vmmap();
	
	
		syscall(315, &local_var, (void*)&pfn_local_var);
		syscall(315, &global_var_data, (void*)&pfn_global_var_data);
		syscall(315, &global_var_bss, (void*)&pfn_global_var_bss);
		syscall(315, &heap_var, (void*)&pfn_heap_var);	
	
		printf("\n");
		printf("global_var_data = %d, address = %p, pfn = 0x%lx \n", global_var_data, &global_var_data, pfn_global_var_data);
		printf("\n");
		printf("global_var_bss = %d, address = %p, pfn = 0x%lx \n",global_var_bss, &global_var_bss,pfn_global_var_bss);
		printf("\n");
		printf("local_var = %d, address = %p, pfn = 0x%lx \n", local_var, &local_var, pfn_local_var);
		printf("\n");
		printf("heap_var = %d, address = %p, pfn = 0x%lx \n", *heap_var, heap_var, pfn_heap_var);
		printf("\n");
		sleep(4);

		lib_physical(global_var_data, "libpthread-2.19.so");
		lib_physical(global_var_data, "process1");
		lib_physical(global_var_data, "libc-2.19.so");
		lib_physical(global_var_data, "ld-2.19.so");
		stack_physical(global_var_data, &local_var);
		heap_physical(global_var_data);
		code_physical(global_var_data);
		data_physical(global_var_data);
		print_vmmap(global_var_data);
		sleep(20);}
	
	return 0;
}
```

## System call 314
```c
#include <linux/kernel.h>

#include <linux/mm.h>

#include <linux/syscalls.h>

#include <asm/current.h>


static int is_data(struct vm_area_struct *vma)

{

	return  vma->vm_start <= vma->vm_mm->end_data &&

			vma->vm_end >= vma->vm_mm->start_data;

}
static int is_code(struct vm_area_struct *vma)

{

	return  vma->vm_start <= vma->vm_mm->end_code &&

			vma->vm_end >= vma->vm_mm->start_code;

}

static int is_stack(struct vm_area_struct *vma)

{

	return  vma->vm_start <= vma->vm_mm->start_stack &&

			vma->vm_end >= vma->vm_mm->start_stack;

}



static int is_heap(struct vm_area_struct *vma)

{

	return  vma->vm_start <= vma->vm_mm->brk &&

			vma->vm_end >= vma->vm_mm->start_brk;

}



struct map{

	unsigned long start;	

	unsigned long end;

	char flags[5];

	char name[40];

	unsigned long physical;
	unsigned long physical_end;

};



//asmlinkage int sys_my_vmmap(int l, void* __user user_address)

SYSCALL_DEFINE2(my_vmmap, int, l, void* __user, user_address)

{

	

	struct map my_map;

	struct vm_area_struct *now= current -> mm -> mmap;

	int i;	

	for(i=0; i<l; i++){

		if(now == NULL) return 0;

		now = now->vm_next;

	}

	if(now == NULL) return 0;

	my_map.start = now->vm_start;

	my_map.end = now->vm_end;

	my_map.flags[0] = now->vm_flags & VM_READ ? 'r' : '-';

	my_map.flags[1] = now->vm_flags & VM_WRITE ? 'w' : '-';

	my_map.flags[2] = now->vm_flags & VM_EXEC ? 'x' : '-';

	my_map.flags[3] = now->vm_flags & VM_MAYSHARE ? 's' : 'p';

	my_map.flags[4] = '\0';



	/*if(now->vm_file){

		strcpy(my_map.name, now->vm_file->f_path.dentry->d_iname);

	}*/
	


	if(is_code(now)){

		strcpy(my_map.name, "code");

	}

	else if(is_heap(now)){

		strcpy(my_map.name, "main heap");

	}

	else if(is_data(now)){

		strcpy(my_map.name, "data");

	}
	else if(now->vm_file){

		strcpy(my_map.name, now->vm_file->f_path.dentry->d_iname);

	}
	else if(is_stack(now)){

		strcpy(my_map.name, "main stack");

	}

	else{

		strcpy(my_map.name, " ");

	}

	

	my_map.physical=0;
	my_map.physical_end=0;
	

	copy_to_user(user_address, &my_map, sizeof(struct map));

	

	return 1;

 }
 ```
 
 ## System call 315
```c
#include<linux/kernel.h>
#include<linux/syscalls.h>
#include<asm/current.h>
#include<asm/pgtable.h>
#include<asm/highmem.h>

/*unsigned long vaddr_to_paddr(unsigned long vaddr){
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;
	unsigned long pfn = 0;		     
	unsigned long offset = 0;
    unsigned long paddr = 0;
	
	// cr3 -> pgd 
        pgd = pgd_offset(current->mm,  vaddr);
	printk("cr3 -> pgd\n");
	printk("pgd_val = 0x%lx\n", pgd_val(*pgd));
    	printk("pgd_index = %lu\n", pgd_index(vaddr));
	printk("PGDIR_SHIFT = %d\n", PGDIR_SHIFT);
	printk("PTRS_PER_PGD = %d\n", PTRS_PER_PGD);
	printk("\n");
	if (pgd_none(*pgd)) {
        	printk("virtual address: %lx not mapped in pgd\n", vaddr);
        	return 0;
    	}

	// pgd -> pud
        pud = pud_offset(pgd, vaddr);
	printk("pgd -> pud\n");	
    	printk("pud_val = 0x%lx\n", pud_val(*pud));
	printk("PUD_SHIFT = %d\n", PUD_SHIFT);
	printk("PTRS_PER_PUD = %d\n", PTRS_PER_PUD);
	printk("\n");
	if (pud_none(*pud)) {
        	printk("virtual address: %lx not mapped in pud\n", vaddr);
        	return 0;
    	}

	// pud -> pmd
        pmd = pmd_offset(pud, vaddr);
	printk("pud -> pmd\n");	
	printk("pmd_val = 0x%lx\n", pmd_val(*pmd));
    	printk("pmd_index = %lu\n", pmd_index(vaddr));
	printk("PMD_SHIFT = %d\n", PMD_SHIFT);
	printk("PTRS_PER_PMD = %d\n", PTRS_PER_PMD);
	printk("\n");
	if (pmd_none(*pmd)) {
        	printk("virtual address: %lx not mapped in pmd\n", vaddr);
        	return 0;
    	}

	// pmd -> pte
	pte = pte_offset_kernel(pmd, vaddr);
	printk("pmd -> pte\n");
	printk("pte_val = 0x%lx\n", pte_val(*pte));
    	printk("pte_index = %lu\n", pte_index(vaddr));
	printk("PAGE_SHIFT = %d\n", PAGE_SHIFT);
	printk("PTRS_PER_PTE = %d\n", PTRS_PER_PTE);
	printk("\n");
	if (pte_none(*pte)) {
        	printk("virtual address: %lx not mapped in pte\n", vaddr);
        	return 0;
    	}

 	// pte -> paddr
	pfn = pte_val(*pte) & PAGE_MASK;
	offset = vaddr & ~PAGE_MASK;
	paddr = pfn | offset;
	printk("pte -> paddr\n");
	printk("virtual address: %lx --> physical address: %lx\n", vaddr, paddr);
	printk("\n");

        return paddr;
	
}*/

SYSCALL_DEFINE2(my_get_physical_addresses, unsigned long, vir, void* __user, user_address)
{	
	struct page *pg;
	unsigned long flags = 0;
	unsigned long phy = 0, pfn = 0, offset = 0;
	

	pg = follow_page(current->mm->mmap, vir, flags);
	pfn = page_to_pfn(pg) << 12;
	offset = vir & ~PAGE_MASK;
	phy = pfn | offset;
	
	copy_to_user(user_address, &phy, sizeof(phy));
    
	return 1;
}
```
    
### References
https://www.cnblogs.com/huxiao-tee/p/4660352.html
https://ithelp.ithome.com.tw/articles/10276664?sc=iThelpR
https://man7.org/linux/man-pages/man2/mmap.2.html

### 補充
1. Process 在 sleep 時的 status 為何 ?
    ![](https://i.imgur.com/3wZrNvc.jpg)
    
    執行程式後下 ps 指令，可以看到目前 process 的 state 為何 (3419,3420)，看了 ps 的 manpage 後，可以知道 S 代表 process 處 interruptible sleep (waiting for an event to complete) 的 state.
    
    
2. 在 terminal 按下 <Ctrl>-C 中止執行中的程式時，是發送了什麼樣的 signal ?
+ 按下 <Ctrl>-C 時， Terminal 會發送了一個 SIGINT(中斷訊號) 給 Shell，Shell 再把 SIGINT 轉發給 process，另外 process 可以自己選擇要不要中止
+ 另外如果按下 <Ctrl>-Z 暫停 process，Terminal 發送的 Signal 則是 SIGTSTP(暫停訊號) 

    
3. mmap()
    mmap 是一種 memory 映射 file 的方法，將一个 file 映射到 process 的 virtual address space，實現 disk 地址和 process virtual address space 中一段 virtual space 的一一對應關係。
    
    mmap 總共分成三個階段，
    （一）process 在 virtual space 中創建虛擬映射區域
    ```c
    void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
    ```
    （二）呼叫mmap系統函數，實現 file 的 physical address 和 process 的 virtual address 的一一對應關係。
    ```c
    int mmap(struct file *filp, struct vm_area_struct *vma)
    ```
    （三）process access 這段 virtual space，通過查詢 page table 後，發現這一段 address 只是建立了 address 的一一映射，disk 中的 data 還沒有被 copy 至 memory，因此發生 page fault，進而將文件內容 copy 到 memory 中。
    
    + 另外我們可以運用 mmap 的 flag 來實現 share memory，也就是讓兩個不同的 process 共用相同的 segment
    ```c
     MAP_SHARED : Share this mapping.  Updates to the mapping are visible to other processes mapping the same region, and (in the case of file-backed mappings) are carried through to the underlying file.
     MAP_PRIVATE : Create a private copy-on-write mapping.  Updates to the mapping are not visible to other processes mapping the same file, and are not carried through to the underlying file.
    ```

4. 什麼時候會發生 page fault ?
+ 使用共享記憶體的區域，但是還不存在虛擬位址與實體位址的對應，而產生的 page fault
+ 訪問的地址在實體記憶體中不存在，需要從硬碟或是 swap 分割槽讀入才能使用
+ 訪問的記憶體位址非法，page fault 會進而引發 SIGSEGN signal 結束程序

5. Copy-On-Write (COW)
    多個 process 要使用相同資料時，在一開始只會 loading 一份資料，並且被標記成 read-only 。當有 process (不論是child process 或是原始的 process) 要寫入時則會對 kernel 觸發 page fault ，進而使得 page fault handler 處理 copy on write 的操作。
---