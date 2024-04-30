#include "./headers/exceptions.h"

extern struct list_head pcbFree_h;
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern struct list_head readyQueue;
//extern struct list_head receiveMessageQueue;

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

	if(current_process != NULL){
		copyState((state_t *)BIOSDATAPAGE, &current_process->p_s);
		// update accumulated cpu time
		current_process->p_time += getTIMER();
	}

	if(excCode == IOINTERRUPTS)
		interruptHandler(cause);
	else if(excCode == SYSEXCEPTION)
		syscallHandler();
	else if(excCode <= 3) // codes 1-3
		passUpOrDie(PGFAULTEXCEPT, &current_process->p_s);
	else if(excCode <= 12) // codes 4-7, 9-12
		passUpOrDie(GENERALEXCEPT, &current_process->p_s); // Trap Handler
	else{
		klog_print("exeption not handled\n");
		breakPoint();
	}
}

/*
SYSCALL(SENDMESSAGE, (unsigned int)destination, (unsigned int)payload, 0);
*/
int sendMessage(pcb_PTR dest, unsigned int payload){
	klog_print("msg sent\n");
	if(dest == SSIADDRESS) dest = ssi_pcb;

	msg_PTR msg = allocMsg();
	if(msg == NULL) return MSGNOGOOD;
	msg->m_sender = current_process;
	msg->m_payload = payload;

	if(searchProcQ(&pcbFree_h, dest) == dest){
		return DEST_NOT_EXIST;
	}else{
		pushMessage(&dest->msg_inbox, msg);
		return 0;
	}
}

/*
SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)payload, 0);
*/
pcb_PTR receiveMessage(pcb_PTR sender, unsigned int payload){
	/* assuming ANYMESSAGE == NULL */
	msg_PTR msg = popMessage(&current_process->msg_inbox, sender);
	if(msg == NULL){
		klog_print("blocking receive\n");
		insertProcQ(&readyQueue, current_process);
		current_process = NULL;
		scheduler();
		return 0; // for compiler
	}else{
		klog_print("msg received\n");
		*&payload = msg->m_payload;
		return msg->m_sender;
	}
}

void syscallHandler(void){
	if((current_process->p_s.status & USERPON) != 0){
		/* reserved instruction PRIVINSTR
		syscall only available in kernel mode */
		passUpOrDie(GENERALEXCEPT, &current_process->p_s); // Trap Handler
	}

	if(current_process->p_s.reg_a0 == SENDMESSAGE){
		current_process->p_s.reg_v0 = sendMessage((pcb_PTR)current_process->p_s.reg_a1, current_process->p_s.reg_a2);
	}else if(current_process->p_s.reg_a0 == RECEIVEMESSAGE){
		current_process->p_s.reg_v0 = (unsigned int)receiveMessage((pcb_PTR)current_process->p_s.reg_a1, current_process->p_s.reg_a2);
	}

	current_process->p_s.pc_epc += WORDLEN;
	LDST(&current_process->p_s);
}

void passUpOrDie(int except_type, state_t *exceptionState) {
	klog_print("passUpOrDie\n");
    if (current_process->p_supportStruct == NULL) { 
        // Die (process termination)
		struct ssi_payload_t payload;
		payload.service_code = TERMPROCESS;
		payload.arg = NULL;
		sendMessage(SSIADDRESS, (unsigned int) &payload);
	}else{ // PassUp
		copyState(exceptionState, &(current_process->p_supportStruct)->sup_exceptState[except_type]); 
        context_t info_to_pass = (current_process->p_supportStruct)->sup_exceptContext[except_type]; // passing up the support info
        LDCXT(info_to_pass.stackPtr, info_to_pass.status, info_to_pass.pc);                   // to the support level
        // LDCXT is used to change the operating mode/context of a process
    }
}
