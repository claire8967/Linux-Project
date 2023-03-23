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