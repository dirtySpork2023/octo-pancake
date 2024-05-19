#include "./headers/interrupts.h"

void klog_print();
void klog_print_dec();
void breakPoint();
extern int softBlockCount;
extern pcb_PTR current_process;
extern pcb_PTR ssi_pcb;
extern struct list_head readyQueue;
extern struct list_head pseudoClockQueue;
extern pcb_PTR devQueue[DEVINTNUM][DEVPERINT];

void interruptHandler(int cause){	

	if(cause & LOCALTIMERINT) {
		processorLocalTimer();
	}else if(cause & TIMERINTERRUPT) {
		intervalTimer();	
	}else{
		deviceInterrupt(cause);	
	}

	if(current_process != NULL){
		// only if there was a process running before the interrupt
		// load processor state stored at address BIOSDATAPAGE
		LDST(EXST);
	}else{
		scheduler();
	}
}

void processorLocalTimer(){
	if(current_process == NULL){
		klog_print("ERR localTimer\n");
		breakPoint();
	}

	setTIMER(NEVER); // redundant operation, will be overwritten by scheduler
	copyState(EXST, &current_process->p_s);
	current_process->p_time += TIMESLICE;
	insertProcQ(&readyQueue, current_process);
	current_process = NULL;
}

void intervalTimer(){
	/* re-set with 100 milliseconds */
	LDIT(PSECOND);
	/*	unlock all pcbs in pseudoClockQueue */
	while(!emptyProcQ(&pseudoClockQueue)){
		softBlockCount--;
		insertProcQ(&readyQueue, removeProcQ(&pseudoClockQueue));
	}
}

unsigned int getDeviceNumber (unsigned int interruptLine) {
    unsigned int intdevBitMap = 0x10000040; // Interrupt Line ? Interrupting Devices Bit Map

    intdevBitMap = intdevBitMap + ((interruptLine - 3) * 0x04);
    //find the device number [Tip: to calculate the device number you can use a switch among constan    ts DEVxON]
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

/*
 * non so se ho sbagliato io a fare merge
 * funzionava da dio prima che lo fixxavi
 * ma se vuoi usare il bit shifting invece dello switch fai pure 
 * 
unsigned int getDeviceNumber (unsigned int interruptLine) {
	unsigned int intdevBitMap = 0x10000040; // Interrupt Line ? Interrupting Devices Bit Map

	intdevBitMap = intdevBitMap + ((interruptLine - 3) * 0x04);
	//find the device number [Tip: to calculate the device number you can use a switch among constants DEVxON]
	unsigned int mask = 0x01;
	
	for (int i = 0; i < 8; i++) {
		if(intdevBitMap & mask) { // the last 8 bits represent the device number
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
}*/

void deviceInterrupt(int cause){

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
	
	unsigned int devNumber = getDeviceNumber(interruptLine);
	
	//ho creato DEVADDR ma mi sono accordo adesso che esiste anche START_DEVREG
	// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
	unsigned int devAddrBase = START_DEVREG + ((interruptLine - 3) * 0x80) + (devNumber * 0x10);
	
	unsigned int devStatus;
	if(interruptLine == TERMINT){
		// terminal device has two command and two status registers
		// both must be handled before acknowledging the interrupt
		// TODO adesso è hard coded solo per trasmettere
		
		// save off the status code from the device’s register
		// from bits 0-7 of TRANSM_STATUS register (base+0x8)
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

	#ifdef DEBUG then	
	klog_print("dev ");
	klog_print_dec(devNumber);
	klog_print(" of ");
	klog_print_dec(interruptLine);
	klog_print(" status ");
	klog_print_dec(devStatus);
	klog_print("\n");
	#endif

	softBlockCount--;
	pcb_PTR requester = devQueue[interruptLine-3][devNumber];	
	
	devQueue[interruptLine-3][devNumber] = NULL;
	
	if(requester != NULL){
		// la risposta viene mandata direttamente da qui ma da parte dell'SSI
		// le specifiche dicono di mandare lo status all'SSI mettendo devaddrbase nel registro a3 :c
		sendMessage(requester, &devStatus, ssi_pcb);
		
		requester->p_s.reg_v0 = devStatus;
		
		insertProcQ(&readyQueue, requester);
	}else{
		klog_print("ERR: requester NULL");
		breakPoint();
	}

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
