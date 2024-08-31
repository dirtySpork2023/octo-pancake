#include "./headers/initProc.h"

// phase 3 global variables
pcb_PTR swap_pcb, sst_pcb[UPROCMAX];

void test(){
	state_t swapState, sstState;
	unsigned int ramtop;
	RAMTOP(ramtop);

	/*/installed devs
	#ifdef DEBUG
	klog_print_hex(0x70000000 + *((memaddr *)(0x1000002C + 0x0C)));
	klog_print_hex(0x60000000 + *((memaddr *)(0x1000002C + 0x10)));
	klog_print_hex(0x40000000 + *((memaddr *)(0x1000002C + 0x04)));
	devreg_t *flashDev = (devreg_t *)(START_DEVREG + (4-3)*0x80 + 0*0x10);
	klog_print_dec(flashDev->dtp.status);
	#endif*/


	// create swap process
	initSwapStructs();
	
	STST(&swapState);
	swapState.reg_sp = ramtop - 4*PAGESIZE; 
	swapState.pc_epc = (memaddr)swapMutex;
    swapState.reg_t9 = (memaddr)swapMutex;
    swapState.status = ALLOFF | IEPON | IMON | TEBITON; // interrupts enabled?
	swap_pcb = newProc(&swapState, NULL);
	
	// create SST processes
	STST(&sstState);
	sstState.reg_sp = swapState.reg_sp; 
	sstState.pc_epc = (memaddr)SST;
    sstState.reg_t9 = (memaddr)SST;
    sstState.status = ALLOFF | IEPON | IMON | TEBITON;

	//temprorary for debug
	support_t supportSST;
	initSupportStruct(&supportSST, 1);

	for(unsigned int asid=0; asid<UPROCMAX; asid++){
		sstState.reg_sp = sstState.reg_sp - QPAGE; 
		sstState.entry_hi = (asid+1) << ASIDSHIFT; // ASID starts from 1
		sst_pcb[asid] = newProc(&sstState, &supportSST);
	}


	// TODO init each peripheral device ?

	// terminate after all SST have terminated
	int uProcCount = UPROCMAX;
	while(uProcCount > 0){
		SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
		uProcCount--;
	}
	klog_print("test terminated\n");
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
    SYSCALL(SENDMESSAGE, SSIADDRESS, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, SSIADDRESS, (unsigned int)&newPCB, 0);
	return newPCB;
}

void suicide(void){
	ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = (void *)NULL, //kill self
    };
    SYSCALL(SENDMESSAGE, SSIADDRESS, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, SSIADDRESS, 0, 0);
}

void initSupportStruct(support_t *supportStruct, unsigned int asid){ 
	unsigned int ramtop;
	RAMTOP(ramtop);
	supportStruct->sup_asid = asid;
	supportStruct->sup_exceptContext[GENERALEXCEPT].pc = (memaddr) generalExceptionHandler;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pageFaultExceptionHandler;
	supportStruct->sup_exceptContext[GENERALEXCEPT].status = IEPON | IMON | TEBITON;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].status = IEPON | IMON | TEBITON;
	supportStruct->sup_exceptContext[GENERALEXCEPT].stackPtr = ramtop - 2 * PAGESIZE; // &(supportStruct->sup_stackGen[499]);
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].stackPtr = ramtop - 3 * PAGESIZE; // &(supportStruct->sup_stackGen[499]);
	
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
