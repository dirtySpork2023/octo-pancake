// support struct context for page fault exceptions points to the Pager in vmSupport.c
// support struct context for general exceptions points to handler in sysSypport.c

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;

//phase 3 global variables
// instantiatorProcess?
void test(){
	initSwapStructs();
	// each peripheral device
	// eight SST for each U-proc

	// terminate after all SST have terminated
}
