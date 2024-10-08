#include "./headers/exceptions.h"

void klog_print();
void klog_print_dec();
void klog_print_hex();
void breakPoint();
void killProcess();
extern struct list_head pcbFree_h;
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern struct list_head readyQueue;
extern struct list_head receiveMessageQueue;

void uTLB_RefillHandler(void){
	/* setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000); */
	
	// prendo solo i primi 20 bit (VPN)
	// required Page number
	unsigned int p = (EXST->entry_hi & GETPAGENO) >> VPNSHIFT;
	if(p >= MAXPAGES) p = MAXPAGES-1; // stack page
	
	#ifdef DEBUG_TLB
	//unsigned int asid = (EXST->entry_hi & GETASID) >> ASIDSHIFT;
	//klog_print("asid = ");
	//klog_print_dec(asid);
	klog_print("\nrefill page = ");
	klog_print_dec(p);
	klog_print("\n");
	//klog_print_hex(getENTRYHI());
	//klog_print(" ");
	//klog_print_hex(getENTRYLO());
	//klog_print("\n");
	#endif

	pteEntry_t pageTableEntry = current_process->p_supportStruct->sup_privatePgTbl[p];
	
	// write this page table entry to the TLB
	setENTRYHI(pageTableEntry.pte_entryHI);
	setENTRYLO(pageTableEntry.pte_entryLO);

	TLBWR(); // random index

	// return control to the current process
	LDST(EXST);
}

/* redirects exceptions to the correct handler */
void exceptionHandler(void){
	/* the processor state at the time of the exception will
	have been stored at BIOS data page */
	/* processor already set to kernel mode and disabled interrupts*/
	unsigned int cause = getCAUSE();
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;

	if(excCode == IOINTERRUPTS)
		interruptHandler(cause);
	else if(excCode == SYSEXCEPTION)
		syscallHandler();
	else if(excCode <= 3) // codes 1-3
		passUpOrDie(PGFAULTEXCEPT, EXST);
	else if(excCode <= 12) // codes 4-7, 9-12
		passUpOrDie(GENERALEXCEPT, EXST); // Trap Handler
	else{
		klog_print("ERR: exception not handled\n");
		breakPoint();
	}
}

void syscallHandler(void){
	#ifdef DEBUG_EXEP
	klog_print("syscall exception\n");
	#endif
	
	if((int)EXST->reg_a0 >= 1)
		passUpOrDie(GENERALEXCEPT, EXST);

	enforceKernelMode();

	if(EXST->reg_a0 == SENDMESSAGE){
		EXST->reg_v0 = sendMessage((pcb_PTR)EXST->reg_a1, &EXST->reg_a2, current_process);
	}else if(EXST->reg_a0 == RECEIVEMESSAGE){
		EXST->reg_v0 = (unsigned int)receiveMessage((pcb_PTR)EXST->reg_a1, (unsigned int *)EXST->reg_a2);
	}else{
		klog_print("ERR strange syscall");
	}
	
	EXST->pc_epc += WORDLEN;
	LDST(EXST);
}

void enforceKernelMode(void){
	if((EXST->status & USERPON) != 0){
		/* syscall only available in kernel mode
		 * change excCode to Reserved Instruction (10) */
		// clear exception code and write PRIVINSTR
		EXST->cause = (getCAUSE() & !GETEXECCODE) | (PRIVINSTR << CAUSESHIFT);	
		klog_print("ERR: syscall not allowed in user mode");
		passUpOrDie(GENERALEXCEPT, EXST); // Trap Handler
	}
}

/*
SYSCALL(SENDMESSAGE, (unsigned int)destination, (unsigned int)payload, 0);
*/
int sendMessage(pcb_PTR dest, unsigned int *payload, pcb_PTR sender){
	if(dest == (pcb_PTR)SSIADDRESS) dest = ssi_pcb;

	msg_PTR msg = allocMsg();
	if(msg == NULL){
		klog_print("ERR: alloc msg failed\n");
		return MSGNOGOOD;	
	}
	
	msg->m_sender = sender;
	msg->m_payload = *payload;
	
	if(searchProcQ(&pcbFree_h, dest) == dest){
		//klog_print("ERR: dest pcb dead\n");
		return DEST_NOT_EXIST;
	}
	/*if(dest != ssi_pcb && sender != ssi_pcb)
		klog_print("Sent ");*/

	/* pcb blocked on receive is put in ready state */
	if(searchProcQ(&receiveMessageQueue, dest) == dest){
		insertProcQ(&readyQueue, outAnyProcQ(dest));
	}

	pushMessage(&dest->msg_inbox, msg);

	return 0;
}

/*
SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)payload, 0);
*/
pcb_PTR receiveMessage(pcb_PTR sender, unsigned int *payload){
	if(sender == (pcb_PTR)SSIADDRESS) sender = ssi_pcb;
	
	/* assuming ANYMESSAGE == NULL */
	msg_PTR msg = popMessage(&current_process->msg_inbox, sender);
	if(msg == NULL){
		/*klog_print("blocking recv ");
		klog_print_dec(current_process->p_pid);
		klog_print("\n");*/
		copyState(EXST, &current_process->p_s);
		current_process->p_time += getTIMER();
		insertProcQ(&receiveMessageQueue, current_process);	
		current_process = NULL;
		scheduler();
		return NULL; // for compiler
	}else{
		/*if(msg->m_sender != ssi_pcb && current_process != ssi_pcb){
			klog_print("msg from ");
			klog_print_dec(msg->m_sender->p_pid);
			klog_print(" to ");
			klog_print_dec(current_process->p_pid);
			klog_print("\n");
		}*/

		freeMsg(msg); // il messaggio rimane accessibile
		if(payload != NULL) *payload = msg->m_payload;
		return msg->m_sender;
	}
}

void passUpOrDie(int except_type, state_t *exceptionState) {
	//klog_print("passUpOrDie\n");
   	if (current_process->p_supportStruct == NULL) {
		#ifdef DEBUG
		klog_print("missing support struct\n");
		#endif
		// Die (process termination)
		killProcess(current_process, current_process);
		/* bisognerebbe usare sendMessage per essere più disponibili a interrupt ma così funziona sicuro
		struct ssi_payload_t payload;
		payload.service_code = TERMPROCESS;
		payload.arg = NULL;
		sendMessage(SSIADDRESS, (unsigned int *)(&payload));*/
		current_process = NULL;
		scheduler();
	}else{ // PassUp
		copyState(exceptionState, &(current_process->p_supportStruct)->sup_exceptState[except_type]); 
        context_t info_to_pass = (current_process->p_supportStruct)->sup_exceptContext[except_type]; // passing up the support info
        LDCXT(info_to_pass.stackPtr, info_to_pass.status, info_to_pass.pc);                   // to the support level
        // LDCXT is used to change the operating mode/context of a process
    }
}
