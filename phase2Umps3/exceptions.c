#include "./headers/exceptions.h"

extern struct list_head pcbFree_h;
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern struct list_head *readyQueue;
extern struct list_head *receiveMessageQueue;

/* to be replaced in phase 3 */
void uTLB_RefillHandler(void){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

/* redirects exceptions to the correct handler */
void exceptionHandler(void){
	/* the processor state at the time of the exception will
	have been stored at BIOS data page */
	/* processor already set to kernel mode and disabled interrupts*/
	unsigned int cause = getCAUSE();
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;

	copyState((state_t *)BIOSDATAPAGE, &current_process->p_s);

	if(excCode == IOINTERRUPTS)
		interruptHandler(cause);
	else if(excCode == SYSEXCEPTION)
		syscallHandler();
	else if(excCode <= 3) // codes 1-3
		{}//TLB exception handler, 
	else if(excCode <= 12) // codes 4-7, 9-12
		programTrapHandler();
	else{
		klog_print("exeption not handled\n");
		breakPoint();
	}
}

void syscallHandler(void){
	if(current_process->p_s.status & USERPON != 0){
		/* reserved instruction PRIVINSTR
		syscall only available in kernel mode */
		programTrapHandler();
	}
	/* information saved in registers: a0, a1, a2, a3 */

	if(current_process->p_s.reg_a0 == SENDMESSAGE){
		current_process->p_s.reg_v0 = sendMessage(current_process->p_s.reg_a1, current_process->p_s.reg_a2);
	}else if(current_process->p_s.reg_a0 == RECEIVEMESSAGE){
		current_process->p_s.reg_v0 = receiveMessage(current_process->p_s.reg_a1, current_process->p_s.reg_a2);
	}

	current_process->p_s.pc_epc += WORDLEN;
	LDST(&current_process->p_s);
}

/*
SYSCALL(SENDMESSAGE, (unsigned int)destination, (unsigned int)payload, 0);
*/
int sendMessage(pcb_PTR dest, unsigned int payload){
	if(dest == SSIADDRESS) dest = ssi_pcb;

	msg_PTR msg = allocMsg();
	if(msg == NULL) return MSGNOGOOD;
	msg->m_sender = current_process;
	msg->m_payload = payload;

	if(searchProcQ(&pcbFree_h, dest) == dest){
		return DEST_NOT_EXIST;
	}else{
		if(searchProcQ(receiveMessageQueue, dest) == dest)
			insertProcQ(readyQueue, outProcQ(receiveMessageQueue, dest));
		pushMessage(&dest->msg_inbox, msg);
		return 0;
	}
}

/*
SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)payload, 0);
*/
int receiveMessage(pcb_PTR sender, unsigned int payload){
	/* assuming ANYMESSAGE == NULL */
	msg_PTR msg = popMessage(&current_process->msg_inbox, sender);
	if(msg == NULL){
		copyState(BIOSDATAPAGE, &current_process->p_s);
		// TODO current_process->p_time += accumulated cpu time??
		insertProcQ(receiveMessageQueue, current_process);
		scheduler();
		return 0; // for compiler
		/* does work with specific sender */
	}else{
		*&payload = msg->m_payload;
		return msg->m_sender;
	}
}

void programTrapHandler(){
	/*	The Nucleus Program Trap exception handler should perform a standard Pass
		Up or Die operation using the GENERALEXCEPT index value. */
	if(currentProcess->p_supportStruct == NULL) {
		// TerminateProcess
	} else {
		//  store the saved exception state at an accessible location known to the Nucleus,
		//  and pass control to a routine specified by the Nucleus
	}
}