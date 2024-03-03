#include "./headers/exceptions.h"

/* to be replaced in phase 3 */
void uTLB_RefillHandler(){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

/* handles all exceptions, not TLB-Refill events */
void exceptionHandler(void){
	/* the processor state at the time of the exception will
have been stored at the start of the BIOS Data Page */
	/* processor already set to kernel mode and disabled interrupts*/
	unsigned int cause = getCAUSE();
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;
	
	/* "break" command will not be executed if kernel works properly */
	switch(excCode){
		case IOINTERRUPTS:
			interruptHandler(cause);
			break;
		case TLBINVLDL:

			break;
		case TLBINVLDS:

			break;
		case SYSEXCEPTION:

			break;
		case BREAKEXCEPTION:

			break;
		case PRIVINSTR:

			break;
		default:
			break;
	}

	
}