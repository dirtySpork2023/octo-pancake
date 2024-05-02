#include "./headers/interrupts.h"

extern int softBlockCount;
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern struct list_head readyQueue;
extern struct list_head pseudoClockQueue;
extern pcb_PTR devQueue[DEVINTNUM][DEVPERINT];

unsigned int getDeviceNumber (unsigned int interruptLine) {
	unsigned int intdevBitMap = 0x10000040; // Interrupt Line ? Interrupting Devices Bit Map

	intdevBitMap = intdevBitMap + ((interruptLine - 3) * 0x04);
	//find the device number [Tip: to calculate the device number you can use a switch among constants DEVxON]
	unsigned int mask = 0x01;

	for (int i = 0; i < 8; i++) {
		switch(intdevBitMap & mask) { // the last 8 bits represent the device number
			case DEV0ON: return 0;
			case DEV1ON: return 1;
			case DEV2ON: return 2;
    		case DEV3ON: return 3;
    		case DEV4ON: return 4;
    		case DEV5ON: return 5;
    		case DEV6ON: return 6;
    		case DEV7ON: return 7;
		}
		mask = mask << 1;
	}
	// if all cases fail, default to 0
	return 0;
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
	
	klog_print("device interrupt\n");

	// TODO priority within same interrupt line ?
	unsigned int interruptLine;	
	if(cause & DISKINTERRUPT)
		interruptLine = DISKINT;
	else if(cause & FLASHINTERRUPT)
		interruptLine = FLASHINT;	
	else if(cause & PRINTINTERRUPT)
		interruptLine = PRNTINT;
	else if(cause & TERMINTERRUPT)
		interruptLine = TERMINT;
	else{
		/* interrupt lines 0 and 5*/
		klog_print("ERROR interrupts.c\n");
		breakPoint();
	}
	
	klog_print("interrupt line = ");
	klog_print_dec(interruptLine);
	klog_print("\n");
	
	unsigned int devNumber = getDeviceNumber(interruptLine);
	
	klog_print("device number = ");
	klog_print_dec(devNumber);
	klog_print("\n");
	
	// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
	unsigned int devAddrBase = DEVADDR + ((interruptLine - 3) * 0x80) + (devNumber * 0x10);
	devAddrBase = TERMADDR;	
	klog_print("address base = ");
	klog_print_hex(devAddrBase);
	klog_print("\n");
	
	unsigned int devStatus;
	if(interruptLine == TERMINT){
		// terminal device has two command and two status registers
		// both must be handled before acknowledging the interrupt
		// TODO adesso è hard coded solo per trasmettere
		// viene causato kernel panic perchè lo status del device è 0
		
		// save off the status code from the device’s register
		// TRANSM_STATUS = (base) + 0x8 
		devStatus = *((unsigned int *)(devAddrBase + 0x8)) & 0x000000FF; 
		// acknowledge the interrupt
		// TRANSM_COMMAND = (base) + 0xc
		*((unsigned int *)(devAddrBase + 0xc)) = ACK;
	}else{
		// save off the status code from the device’s register
		// STATUS = (base) + 0x0
		devStatus = *((unsigned int *)devAddrBase);
	
		// acknowledge the interrupt
		// COMMAND = (base) + 0x4)
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;
	}
	
	klog_print("device status = ");
	klog_print_dec(devStatus);
	klog_print("\n");
	
	pcb_PTR requester = devQueue[interruptLine-3][devNumber];	
	devQueue[interruptLine-3][devNumber] = NULL;
	if(requester != NULL){
		requester->p_s.reg_v0 = devStatus;
		// la risposta dev'essere da parte della SSI
		//pcb_PTR tmp = current_process;
		//current_process = ssi_pcb;
		{
			pcb_PTR current_process = ssi_pcb;
			sendMessage(requester, devStatus);
		}
		//current_process = tmp;
		insertProcQ(&readyQueue, requester);
	}
	LDST((state_t *)BIOSDATAPAGE);
	

	/*
	//find interrupt lines active in order of priority
	if(cause & DISKINTERRUPT) {
		// find the address for this device's device register
		devNumber = getDeviceNumber(DISKINT);
		// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
		devAddrBase = DEVADDR + ((DISKINT - 3) * 0x80) + (devNumber * 0x10);

		// save off the status code from the device’s device register
		// (base) + 0x0
		devStatus = *((unsigned int *)devAddrBase);

		//acknowledge the interrupt (COMMAND is found at address (base) + 0x4)
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;

		requester = devQueue[DISKINT][devNumber];	
	    devQueue[DISKINT][devNumber] = NULL;
		if(requester != NULL){
			requester->p_s.reg_v0 = devStatus;
			sendMessage(requester, devStatus);
			insertProcQ(&readyQueue, requester);
		}
		LDST((state_t *)BIOSDATAPAGE);
	}

	if(cause & FLASHINTERRUPT){
		devNumber = getDeviceNumber(FLASHINT);
		devAddrBase = DEVADDR + ((FLASHINT - 3) * 0x80) + (devNumber * 0x10);
		devStatus = *((unsigned int *)devAddrBase);
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;
		// ...
	}

	if(cause & PRINTINTERRUPT){
		devNumber = getDeviceNumber(PRNTINT);
		devAddrBase = 0x10000054 + ((PRNTINT - 3) * 0x80) + (devNumber * 0x10);
		devStatus = *((unsigned int *)devAddrBase);
		*((unsigned int *)(devAddrBase + 0x4)) = ACK;
		// ...
		
	}
	if(cause & TERMINTERRUPT){
		// terminal devices have different formats
		
	}*/
}
