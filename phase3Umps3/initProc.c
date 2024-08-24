#include "./headers/initProc.h"

// pagesize is 4096
#define QPAGE 1024
#define DEBUG

// phase 3 global variables
pcb_PTR swap_pcb, sst_pcb[UPROCMAX];

void test(){
	state_t swapState, sstState;
	//support_t sstSupport;
	unsigned int ramtop;
	RAMTOP(ramtop);


	// create swap process
	initSwapStructs();
	
	STST(&swapState);
	swapState.reg_sp = ramtop - 3*PAGESIZE; 
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
	
	for(unsigned int asid=0; asid<UPROCMAX; asid++){
		sstState.reg_sp = sstState.reg_sp - QPAGE; 
		sstState.entry_hi = (asid+1) << ASIDSHIFT; // ASID starts from 1
		sst_pcb[asid] = newProc(&sstState, NULL);
	}


	// TODO init each peripheral device


	// terminate after all SST have terminated
	int uProcCount = 8;
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
	unsigned int supportStack;
	RAMTOP(supportStack);
	supportStruct->sup_asid = asid;
	supportStruct->sup_exceptContext[GENERALEXCEPT].pc = (memaddr) generalExceptionHandler;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pageFaultExceptionHandler;
	supportStruct->sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
	supportStruct->sup_exceptContext[GENERALEXCEPT].stackPtr = supportStack - PAGESIZE; // &(supportStruct->sup_stackGen[499]);
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].stackPtr = supportStack - 2 * PAGESIZE; // &(supportStruct->sup_stackGen[499]);
	// sup_privatePgTbl[USERPGTBLSIZE] ? TODO
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
