#include "./headers/initProc.h"

void newProc(state_t *procState, support_t *procSupport){
	ssi_create_process_t ssi_create_process = {
		.state = procState,
        .support = procSupport,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&child_pcb), 0);
}

void suicide(void){
	ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = (void *)NULL, //kill self
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void initSupportStruct(support_t *supportStruct, unsigned int asid){ 
	supportStruct->sup_asid = asid;
	supportStruct->sup_exceptContext[GENERALEXCEPT].pc = (memaddr) generalExceptionHandler;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pageFaultExceptionHandler;
	supportStruct->sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPBITON | CAUSEINTMASK | TEBITON;
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPBITON | CAUSEINTMASK | TEBITON;
	supportStruct->sup_exceptContext[GENERALEXCEPT].stackPtr = &(supportStruct.sup_stackGen[499]);
	supportStruct->sup_exceptContext[PGFAULTEXCEPT].stackPtr = &(supportStruct.sup_stackGen[499]);
	// sup_privatePgTbl[USERPGTBLSIZE] ? TODO
}

//phase 3 global variables
// instantiatorProcess?
void test(){
	initSwapStructs();
	// TODO init each peripheral device
	// TODO init eight SST for each U-proc

	// terminate after all SST have terminated
	int uProcCount = 8;
	while(uProcCount > 0){
		SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
		uProcCount--;
	}
	klog_print("test terminated\n");
	suicide();	
}
