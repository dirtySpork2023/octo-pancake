#include "./headers/interrupts.h"

extern int softBlockCount;
extern pcb_PTR current_process;
extern struct list_head readyQueue;
extern struct list_head pseudoClockQueue;


unsigned int getDeviceNumber (unsigned int interruptLine) {
	unsigned int intdevBitMap = 0x10000040; // Interrupt Line ? Interrupting Devices Bit Map

	intdevBitMap = intdevBitMap + ((interruptLine - 3) * 0x04);
	//find the device number [Tip: to calculate the device number you can use a switch among constants DEVxON]
	switch(intdevBitMap & 0x000000FF) { // the last 8 bits represent the device number
		case DEV0ON: return 0;
		case DEV1ON: return 1;
		case DEV2ON: return 2;
    	case DEV3ON: return 3;
    	case DEV4ON: return 4;
    	case DEV5ON: return 5;
    	case DEV6ON: return 6;
    	case DEV7ON: return 7;
		default: return 0;
	}
}

void interruptHandler(int cause){
	
	/* Processor Local Timer */
	if(cause & LOCALTIMERINT) {	
		current_process->p_time += TIMESLICE;
		insertProcQ(&readyQueue, current_process);
		current_process = NULL;
		scheduler();
	}
	
	/* Interval Timer */
	if(cause & TIMERINTERRUPT) {
		/* re-set with 100 milliseconds */
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
	
	// TODO priority within same interrupt line ?
	unsigned int interruptLine;	
	if(cause & DISKINTERRUPT)
		interruptLine = DISKINT;
	else if(cause & FLASHINTERRUPT)
		interruptLine = FLASHINT;	
	else if(cause & PRINTINTERRUPT)
		interruptLine = PRNTINT;
	else if(cause & TERMINTERRUPT)
		interruptLine = TERMINT
	else{
		/* interrupt lines 0 and 5*/
		klog_print("ERROR interrupts.c\n");
		breakPoint();
	}

	unsigned int deviceNumber = getDeviceNumber(interruptLine);
	
	// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
	unsigned int devAddrBase = DEVADDR + ((interruptLine - 3) * 0x80) + (deviceNumber * 0x10);

	unsigned int devStatus;
	if(interruptLine == TERMINT){
		//terminal device has two command and two status registers
		//both must be handled before acknowledging the interrupt
		//TODO save off the status code from the device’s device register
		//TODO acknowledge the interrupt
	}else{
		// save off the status code from the device’s device register
		// (base) + 0x0
		devStatus = *((unsigned int *)devAddrBase);

		// acknowledge the interrupt (COMMAND is found at address
		// (base) + 0x4)
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;
	}
	
	requester = devQueue[interruptLine][deviceNumber];	
	devQueue[interruptLine][deviceNumber] = NULL;
	if(requester != NULL){
		requester->p_s.reg_v0 = devStatus;
		sendMessage(requester, devStatus);
		insertProcQ(&readyQueue, requester);
	}
	LDST((state_t *)BIOSDATAPAGE);
	

	/*
	//find interrupt lines active in order of priority
	if(cause & DISKINTERRUPT) {
		// find the address for this device's device register
		deviceNumber = getDeviceNumber(DISKINT);
		// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
		devAddrBase = DEVADDR + ((DISKINT - 3) * 0x80) + (deviceNumber * 0x10);

		// save off the status code from the device’s device register
		// (base) + 0x0
		devStatus = *((unsigned int *)devAddrBase);

		//acknowledge the interrupt (COMMAND is found at address (base) + 0x4)
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;

		requester = devQueue[DISKINT][deviceNumber];	
	    devQueue[DISKINT][deviceNumber] = NULL;
		if(requester != NULL){
			requester->p_s.reg_v0 = devStatus;
			sendMessage(requester, devStatus);
			insertProcQ(&readyQueue, requester);
		}
		LDST((state_t *)BIOSDATAPAGE);
	}

	if(cause & FLASHINTERRUPT){
		deviceNumber = getDeviceNumber(FLASHINT);
		devAddrBase = DEVADDR + ((FLASHINT - 3) * 0x80) + (deviceNumber * 0x10);
		devStatus = *((unsigned int *)devAddrBase);
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;
		// ...
	}

	if(cause & PRINTINTERRUPT){
		deviceNumber = getDeviceNumber(PRNTINT);
		devAddrBase = 0x10000054 + ((PRNTINT - 3) * 0x80) + (deviceNumber * 0x10);
		devStatus = *((unsigned int *)devAddrBase);
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;
		// ...
		
	}
	if(cause & TERMINTERRUPT){
		// terminal devices have different formats
		
	}*/
}
