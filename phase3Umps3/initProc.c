#include "./headers/initProc.h"

// phase 3 global variables
pcb_PTR swap_pcb, sst_pcb[UPROCMAX];

support_t supStruct[UPROCMAX];

void test(){
	state_t state;
	unsigned int ramtop;
	RAMTOP(ramtop);

	// create swap process
	initSwapStructs();
	
	STST(&state);
	state.reg_sp = ramtop - 2*PAGESIZE; 
	state.pc_epc = (memaddr)swapMutex;
    state.reg_t9 = (memaddr)swapMutex;
    state.status = IEPON | IMON | TEBITON; // interrupts enabled?
	swap_pcb = newProc(&state, NULL);
	
	// create SST processes
	state.reg_sp = state.reg_sp; 
	state.pc_epc = (memaddr)SST;
    state.reg_t9 = (memaddr)SST;
    state.status = IEPON | IMON | TEBITON;

	for(unsigned int asid=1; asid<=UPROCMAX; asid++){ // asid 0 is reserved for kernel processes
		initSupportStruct(&supStruct[asid-1], asid, state.reg_sp - PAGESIZE);
		state.reg_sp = state.reg_sp - 3*PAGESIZE; 
		state.entry_hi = asid << ASIDSHIFT;
		sst_pcb[asid-1] = newProc(&state, &supStruct[asid-1]);
	}

	#ifdef DEBUG
	extern unsigned int swapPoolStart;
	unsigned int lastStackPage = state.reg_sp - PAGESIZE;
	unsigned int swapPoolEnd = swapPoolStart + sizeof(swap_t) * POOLSIZE;
	if(lastStackPage < swapPoolEnd){
		klog_print("ERR: ram insufficient\n");
		breakPoint();
	}
	#endif

	// TODO init each peripheral device ?

	// terminate after all SST have terminated
	int uProcCount = UPROCMAX;
	while(uProcCount > 0){
		#ifdef DEBUG
		klog_print_dec(uProcCount);
		klog_print(" uProc left\n");
		#endif
		SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
		uProcCount--;
	}
	#ifdef DEBUG
	klog_print("test terminated\n");
	#endif
	suicide();
}

pcb_PTR newProc(state_t *procState, support_t *procSupport){
	pcb_PTR newPCB;
	ssi_create_process_t ssi_create_process = {
		.state = procState,
        .support = procSupport,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)&newPCB, 0);
	return newPCB;
}

void suicide(void){
	ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = (void *)NULL, //kill self
    };
    SYSCALL(SENDMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)SSIADDRESS, 0, 0);
}

void initSupportStruct(support_t *supportStruct, unsigned int asid, unsigned int stack){ 
	supportStruct->sup_asid = asid;
	supportStruct->sup_exceptContext[GENERALEXCEPT].pc = (memaddr) generalExceptionHandler;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pageFaultExceptionHandler;
	supportStruct->sup_exceptContext[GENERALEXCEPT].status = IEPON | IMON | TEBITON;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].status = IEPON | IMON | TEBITON;
	supportStruct->sup_exceptContext[GENERALEXCEPT].stackPtr = stack; // &(supportStruct->sup_stackGen[499]);
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].stackPtr = stack - PAGESIZE; // &(supportStruct->sup_stackGen[499]);
	
	int i;
	for(i=0; i<USERPGTBLSIZE-1; i++){
		supportStruct->sup_privatePgTbl[i].pte_entryHI = KUSEG + (i << VPNSHIFT) + (asid << ASIDSHIFT);
		supportStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON; // valid off
	}
	supportStruct->sup_privatePgTbl[i].pte_entryHI = 0xBFFFF000 + (asid << ASIDSHIFT);
	supportStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON; // valid off
}

support_t *getSupportStruct(){
	ssi_payload_t support_str_payload = {
		.service_code = GETSUPPORTPTR,
		.arg = NULL,
	};
	support_t *supportStruct;

	SYSCALL(SENDMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)&support_str_payload, 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&supportStruct), 0);
	
	return supportStruct;
}
