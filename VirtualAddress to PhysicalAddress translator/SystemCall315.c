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