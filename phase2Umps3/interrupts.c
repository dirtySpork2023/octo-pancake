#include "./headers/interrupts.h"

extern int softBlockCount;
extern pcb_PTR current_process;
extern struct list_head readyQueue;
extern struct list_head pseudoClockQueue;

void interruptHandler(int cause){

	/* timer interrupts */

	if(cause & LOCALTIMERINT) {
		copyState((state_t *)BIOSDATAPAGE, &current_process->p_s); // bisogna farlo questo?
		insertProcQ(&readyQueue, current_process);
		current_process = NULL;
		scheduler();
	}

	if(cause & TIMERINTERRUPT) {
		/* re-set Interval Timer with 100 milliseconds */
		LDIT(PSECOND);
		/*	unlock all pcbs in pseudoClockQueue */
		while(!emptyProcQ(&pseudoClockQueue)){
			softBlockCount--;
			insertProcQ(&readyQueue, removeProcQ(&pseudoClockQueue));
		}
		if(current_process != NULL){
			// only if there was a process running before the interrupt
			// load processor state stored at address BIOSDATAPAGE
			LDST((state_t *)BIOSDATAPAGE);
		}else{
			scheduler();
		}
	}

	/* device interrupts */

	//TODO better management of different interrupts and priority
	//with the way the if conditions are put it should already follow a priority order 
	
	unsigned int devAddrBase;
	unsigned int deviceNumber;
	unsigned int intdevBitMap = 0x10000040; // Interrupt Line 3 Interrupting Devices Bit Map

	//find interrupt lines active in order of priority
	if(cause & DISKINTERRUPT) {

		intdevBitMap = intdevBitMap + ((DISKINT - 3) * 0x04);
		//find the device number [Tip: to calculate the device number you can use a switch among constants DEVxON]
		switch(intdevBitMap & 0x000000FF) { // the last 8 bits represent the device number
			case DEV0ON: deviceNumber = 0; break;
        	case DEV1ON: deviceNumber = 1; break;
        	case DEV2ON: deviceNumber = 2; break;
        	case DEV3ON: deviceNumber = 3; break;
        	case DEV4ON: deviceNumber = 4; break;
        	case DEV5ON: deviceNumber = 5; break;
        	case DEV6ON: deviceNumber = 6; break;
        	case DEV7ON: deviceNumber = 7; break;
			default: deviceNumber = 0;
		}
		// find the address for this device's device register
		// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
		devAddrBase = 0x10000054 + ((DISKINT - 3) * 0x80) + (deviceNumber * 0x10);

		//save off the status code from the device’s device register
		// (base) + 0x0
		unsigned int devStatus = *((unsigned int *)devAddrBase);

		//acknowledge the interrupt (command is found at address (base) + 0x4)
		*((unsigned int *)(devAddrBase + 0x4)) = ACK; //??

		//TODO points 4-7
		//sendMessage()
	}
	//TODO apply to all non-timer interrupts


	if(cause & FLASHINTERRUPT){
		
	}
	if(cause & PRINTINTERRUPT){
		
	}
	if(cause & TERMINTERRUPT){

	}

	/* interrupt lines 0 and 5 are ignored */
	
	klog_print("interrupt not handled\n");
	breakPoint();
}
