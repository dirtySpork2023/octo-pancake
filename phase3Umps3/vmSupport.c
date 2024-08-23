#include "./headers/vmSupport.h"

struct swap_t* swap_table[POOLSIZE];

void initSwapStructs() {
	// swap table (RAMSTART + (32 * PAGESIZE)); 

	// swap_table[0] = (memaddr)(RAMSTART + (32 * PAGESIZE));

	for (int i = 0; i < POOLSIZE; i++) {
		swap_table[i]->sw_asid = NOPROC; // frame unoccupied
	}
}

void swapMutex(){
	unsigned int msg;
	pcb_PTR sender;
	while(TRUE){
		sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, &msg, 0);
		if(msg = RELEASEMUTEX)
			continue;
		SYSCALL(SENDMESSAGE, sender, 0, 0);
		// mutex is given here
		SYSCALL(RECEIVEMESSAGE, sender, &msg, 0);
		if(msg != RELEASEMUTEX) klog_print("mutex error\n");
	}
}

/*
// ho dei dubbi
void waitsignal() {
    unsigned int payload = NULL;
	pcb_PTR sender;

    while(TRUE) {
        sender = (pcb_PTR)SYSCALL(RECEIVEMSG, ANYMESSAGE, (unsigned int)(&payload), 0);
        if (&payload == 3) P(sender);
        else if (&payload == 4) V(sender);
    }
}

void P(pcb_PTR sender) {
    if (swapMutex > 0) {
        swapMutex--;
    } else {
        insertProcQ(&receiveMessageQueue, &sender);
        // wait? ...

    }
    SYSCALL(SENDMSG, (unsigned int)swap_pcb, 0, 0);
}

void V(pcb_PTR sender) {
    if (emptyProcQ(&receiveMessageQueue)) {
        swapMutex++;

    } else {
        // sveglia ? ...
        sender = removeProcQ(&receiveMessageQueue);
    }
    SYSCALL(SENDMSG, (unsigned int)swap_pcb, 0, 0);
}*/

// exception codes 1-3 are passed up to here
// TLB entry found but invalid =>
// TLB_exception_handler
void pageFaultExceptionHandler() {

	ssi_payload_t support_str_payload = {
		.service_code = GETSUPPORTPTR,
		.arg = NULL,
	};
	support_t* supStruct;

	SYSCALL(SENDMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)&support_str_payload, 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&supStruct), 0);
	
	
	state_t* excState = &supStruct->sup_exceptState[PGFAULTEXCEPT];
	// if it's a TLB-Modification we treat it as a program trap
	if (excState->cause == TLBMOD) {
		// if its in mutual exclusion release it (sendMessage)
		
		// TODO Program trap, Section 9 in sysSupport.c
        programTrapsHandler();
	}
	
	// gain mutual exclusion over the swap table
	SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, GETMUTEX, 0); // ask for mutex
	SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_pcb, 0, 0); // wait for it

	// missing page number
	unsigned int p = (excState->entry_hi & GETPAGENO) >> VPNSHIFT;

	static int index = 0;
	int i;
    i = index;
	index = (index + 1) % POOLSIZE;
 
	// if frame i is occupied
	if (swap_table[i]->sw_asid != NOPROC) {
		// operations done atomically by disabling interrupts
	    unsigned int status = getSTATUS();
		setSTATUS(status & DISABLEINTS);
		
		// mark it as non valid means V bit is off
		swap_table[i]->sw_pte->pte_entryLO &= 0xFFFFFDFF; //== ~VALIDON
		
		//update the TLB
        /* TODO after all other aspects of the Support Level are completed/debugged).
        Probe the TLB (TLBP) to see if the newly updated TLB entry is indeed cached in the TLB. If so (Index.P is 0), 
        rewrite (update) that entry (TLBWI) to match the entry in the Page Table */
        TLBCLR();

		//re-enable interrupts
		setSTATUS(status);
		
		//update flash drive
        // 1
        

        // 2, doIO service
        // DATA0 (base) + 0x8
        unsigned int response;

        ssi_do_io_t do_io_struct = {
            //.commandAddr = , 
            .commandValue = FLASHWRITE,
        };
        ssi_payload_t payload = {
            .service_code = DOIO,
            .arg = &do_io_struct,
        };

        SYSCALL(SENDMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&payload), 0);
		SYSCALL(RECEIVEMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&response), 0);
        
        // if errors, treat as program trap
        if (response == 2 || response >= 4) {
            programTrapsHandler();
        }
	}
    // read the content of cp backing store
    unsigned int response;

    ssi_do_io_t do_io_struct = {
        //.commandAddr = , 
        .commandValue = FLASHREAD,
    };
    //...

    if (response == 2 || response >= 4) {
        programTrapsHandler();
    }

    // update the swap pool table's entry i
    // swap_table[i]->sw_pte = ;
    swap_table[i]->sw_pageNo = p;
    swap_table[i]->sw_asid = current_process->p_supportStruct->sup_asid;

    // update the process' page table
    // current_process->p_supportStruct->sup_privatePgTbl[p].pte_entryLO = ;

    // update tlb
    TLBCLR(); // to modify when all is done

    // release mutual exclusion
    SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0);

    LDST(&current_process->p_s);
}

/* Since reading and writing to each U-proc’s flash device is limited to supporting paging, this module should
also contain the function(s) for reading and writing flash devices.*/
