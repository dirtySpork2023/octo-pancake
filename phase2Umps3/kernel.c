#include "./headers/kernel.h"

int main(){
	passupvector_t *passupvector = PASSUPVECTOR;
	passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
	passupvector->tlb_refill_stackPtr = KERNELSTACK;
	passupvector->exception_handler = (memaddr)exceptionHandler;
	passupvector->exception_stackPtr = KERNELSTACK;

	initPcbs();
	initMsgs();
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	mkEmptyProcQ(readyQueue);
	LDIT(PSECOND);

	// first test process
	pcb_PTR pcb1 = allocPcb();
	insertProcQ(readyQueue, pcb1);
	processCount++;
	pcb1->p_s.status &= !IEPON; // interrupt enabled (== interrupt mask disabled)
	pcb1->p_s.status &= !USERPON; // user mode disabled
	pcb1->p_s.pc_epc = (memaddr) test;
	pcb1->p_s.reg_t9 = (memaddr) test;
	RAMTOP(pcb1->p_s.reg_sp); // stack pointer = RAMTOP

	// second test process
	pcb_PTR pcb2 = allocPcb();
	insertProcQ(readyQueue, pcb2);
	processCount++;
	pcb1->p_s.status &= !IEPON; // interrupt enabled (== interrupt mask disabled)
	pcb1->p_s.status &= !USERPON; // user mode disabled
	//pcb1->p_s.status   TEBITON; // local timer ?
	pcb1->p_s.pc_epc = (memaddr) test;
	pcb1->p_s.reg_t9 = (memaddr) test;
	RAMTOP(pcb1->p_s.reg_sp); // stack pointer = RAMTOP
	pcb1->p_s.reg_sp -= 2*FRAMESIZE;

	scheduler();
}