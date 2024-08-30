#include "./headers/vmSupport.h"

struct swap_t swap_table[POOLSIZE];
const unsigned int swapPoolStart = 0x20020000; // RAMSTART + (32 * PAGESIZE) // should be #define

void initSwapStructs() {
	for (int i = 0; i < POOLSIZE; i++) {
		swap_table[i].sw_asid = NOPROC; // frame unoccupied
	}
}

void swapMutex(){
	klog_print("swap pcb started\n");
	unsigned int msg;
	pcb_PTR sender;
	while(TRUE){
		sender = (pcb_PTR)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)&msg, 0);
		if(msg == RELEASEMUTEX)
			continue;
		SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
		klog_print("mutex given\n");
		// mutex is given here
		SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)&msg, 0);
		if(msg != RELEASEMUTEX) klog_print("mutex error\n");
		klog_print("mutex released\n");
	}
}

// exception codes 1-3 are passed up to here
// TLB entry found but invalid =>
// TLB_exception_handler
void pageFaultExceptionHandler() {
	klog_print("pageFault\n");
	support_t* supStruct = getSupportStruct();	
	state_t* excState = &supStruct->sup_exceptState[PGFAULTEXCEPT];

	// if it's a TLB-Modification we treat it as a program trap
	if (excState->cause == TLBMOD) {
		SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0); // probably useless
        programTrapsHandler();
	}
	
	// gain mutual exclusion over the swap table
	SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, GETMUTEX, 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_pcb, 0, 0);

	// missing page number
	unsigned int p = (excState->entry_hi & GETPAGENO) >> VPNSHIFT;
	
	#ifdef DEBUG
	klog_print("missing page ");
	klog_print_dec(p);
	klog_print("\n");
	#endif

	// page replacement algorithm
	static int f = 0;
	f = (f + 1) % POOLSIZE;

	#ifdef DEBUG
	klog_print("selected frame ");
	klog_print_dec(f);
	klog_print("\n");
	#endif

	// if frame f is occupied
	if (swap_table[f].sw_asid != NOPROC) {
		// logical page number k = swap_table[i].sw_pageNo
		// process x = swap_table[i].sw_asid

		// operations done atomically by disabling interrupts
	    unsigned int status = getSTATUS();
		setSTATUS(status & DISABLEINTS);
		
		// mark it as non valid means V bit is off
		swap_table[f].sw_pte->pte_entryLO &= 0xFFFFFDFF; // ~VALIDON
		
		//update the TLB
        /* TODO after all other aspects of the Support Level are completed/debugged).
        Probe the TLB (TLBP) to see if the newly updated TLB entry is indeed cached in the TLB. If so (Index.P is 0), 
        rewrite (update) that entry (TLBWI) to match the entry in the Page Table */
        TLBCLR(); // invalidates all contents of TLB cache

		//re-enable interrupts
		setSTATUS(status);
		
		//update flash drive
		flashDev(FLASHWRITE, (swap_table[f].sw_pte->pte_entryHI & GETPAGENO) >> VPNSHIFT, f, supStruct->sup_asid);
	}
    // read the content of cp backing store
	flashDev(FLASHREAD, p, f, supStruct->sup_asid);

    // update the swap pool table's entry i
    swap_table[f].sw_pageNo = p;
    swap_table[f].sw_asid = supStruct->sup_asid;
    swap_table[f].sw_pte = &supStruct->sup_privatePgTbl[p]; // p == pageNo == block number del flash drive [1-32]

	// operations done atomically by disabling interrupts
	unsigned int status = getSTATUS();
	setSTATUS(status & DISABLEINTS);
    
	// update the process' page table
    supStruct->sup_privatePgTbl[p].pte_entryLO = (f << VPNSHIFT) + VALIDON; // + DIRTYON pandOS assumes all frames to be dirty
    // update tlb
    TLBCLR(); // to modify when all is done

	//re-enable interrupts
	setSTATUS(status);
	
    // release mutual exclusion
    SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0);

    LDST(excState);
}

//	if cmd is FLASHREAD reads from pageNo and writes to frameNo
// 	if cmd is FLASHWRITE write to pageNo and reads from frameNo
void flashDev(unsigned int cmd, unsigned int pageNo, unsigned int frameNo, unsigned int asid){
	// devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
	devreg_t *flashDev = (devreg_t *)(START_DEVREG + (FLASHINT-3)*0x80 + (asid-1)*0x10);
	// DATA0 = address of physical frame
	flashDev->dtp.data0 = swapPoolStart + (frameNo * PAGESIZE);

	ssi_do_io_t do_io_struct = {
		.commandAddr = &flashDev->dtp.command,
		// block number in high order three bytes and the command to write/read in the lower order byte
		.commandValue = (pageNo << 8) + cmd,
	};
	ssi_payload_t payload = {
		.service_code = DOIO,
		.arg = &do_io_struct,
	};
	unsigned int response;

	SYSCALL(SENDMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&payload), 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)SSIADDRESS, (unsigned int)(&response), 0);
	
	// if errors, treat as program trap
	if (response == 2 || response >= 4) {
		programTrapsHandler();
	}
}
