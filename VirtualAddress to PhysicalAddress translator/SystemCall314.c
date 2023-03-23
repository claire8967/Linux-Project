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