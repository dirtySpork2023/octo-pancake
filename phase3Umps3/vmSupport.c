#include "./headers/vmSupport.h"

struct swap_t swap_table[POOLSIZE];
const unsigned int swapPoolStart = 0x20020000; // RAMSTART + (32 * PAGESIZE)

void initSwapStructs() {
	for (int i = 0; i < POOLSIZE; i++) {
		swap_table[i].sw_asid = NOPROC; // frame unoccupied
	}
}

void cleanSwapTable(unsigned int asid){
	for (int i=0; i<POOLSIZE; i++){
        if (swap_table[i].sw_asid == asid)
            swap_table[i].sw_asid = NOPROC;
	}
}

void swapMutex(){
	unsigned int msg;
	pcb_PTR sender;
	while(TRUE){
		sender = (pcb_PTR)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)&msg, 0);
		if(msg == RELEASEMUTEX)
			continue;
		SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
		// mutex is given here
		SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)&msg, 0);
		//if(msg != RELEASEMUTEX) klog_print("mutex usage error\n");
	}
}

void disableInterrups(){
	setSTATUS(getSTATUS() & ~IECON);
}

void enableInterrupts(){
	setSTATUS(getSTATUS() | IECON);
}

// exception codes 1-3 are passed up to here
// TLB entry found but invalid =>
// TLB_exception_handler
void pageFaultExceptionHandler() {
	support_t* supStruct = getSupportStruct();	
	state_t* excState = &supStruct->sup_exceptState[PGFAULTEXCEPT];
	
	// if it's a TLB-Modification we treat it as a program trap
	if (excState->cause == TLBMOD) {
		//SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0);
        programTrapsHandler();
	}
	
	// gain mutual exclusion over the swap table
	SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, GETMUTEX, 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_pcb, 0, 0);

	// missing page number
	unsigned int p = (excState->entry_hi & GETPAGENO) >> VPNSHIFT;
	if(p >= MAXPAGES) p = MAXPAGES-1;

	// page replacement algorithm
	unsigned int f = selectFrame();

/*	#ifdef DEBUG_TLB
	klog_print("page fault = ");
	klog_print_dec(p);
	klog_print("\nselected frame = ");
	klog_print_dec(f);
	klog_print("\n");
	#endif*/
	
	// if frame f is occupied
	if (swap_table[f].sw_asid != NOPROC) {
		klog_print("clearing occupied frame\n");

		disableInterrups();
		
		// mark it as non valid means V bit is off
		swap_table[f].sw_pte->pte_entryLO &= ~VALIDON; // valid off
		//update the TLB
		unsigned int tmp = getENTRYHI();
		updateTLB(swap_table[f].sw_pte);
		setENTRYHI(tmp);

		enableInterrupts();
		
		//update flash drive
		flashDev(FLASHWRITE, swap_table[f].sw_pageNo, f, swap_table[f].sw_asid);
	}
    // read the content of backing store
	flashDev(FLASHREAD, p, f, supStruct->sup_asid);

    // update the swap pool table's entry i
    swap_table[f].sw_pageNo = p;
    swap_table[f].sw_asid = supStruct->sup_asid;
    swap_table[f].sw_pte = &supStruct->sup_privatePgTbl[p];

	disableInterrups();

	// update the process' page table
	supStruct->sup_privatePgTbl[p].pte_entryLO = (swapPoolStart + f * PAGESIZE) | VALIDON | DIRTYON;
    // update tlb
	updateTLB(&supStruct->sup_privatePgTbl[p]);
	
	enableInterrupts();
	
    // release mutual exclusion
    SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0);

    LDST(excState);
}

//	if cmd is FLASHREAD reads from pageNo and writes to frameNo
// 	if cmd is FLASHWRITE writes to pageNo and reads from frameNo
void flashDev(unsigned int cmd, unsigned int pageNo, unsigned int frameNo, unsigned int asid){
	#ifdef DEBUG_TLB
	if(cmd == FLASHWRITE)
		klog_print("flashWrite VPN=");
	else
		klog_print("flashRead VPN=");
	klog_print_dec(pageNo);
	klog_print(" PFN=");
	klog_print_dec(frameNo);
	klog_print("\n");	
	#endif
	
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
	if (response == 2 || response >= 4){
		klog_print("ERR: flash ");
		klog_print_dec(response);
		klog_print("\n");
		programTrapsHandler();
	}
}

/* Probe the TLB (TLBP) to see if the newly updated TLB entry is indeed cached in the TLB. If so (Index.P is 0), 
rewrite (update) that entry (TLBWI) to match the entry in the Page Table */
void updateTLB(pteEntry_t *e){
	setENTRYHI(e->pte_entryHI);
	TLBP();
	if((getINDEX() & PRESENTFLAG) == 0){
		setENTRYHI(e->pte_entryHI);
		setENTRYLO(e->pte_entryLO);
		TLBWI();
	}
}

unsigned int selectFrame(){
	static unsigned int f = 0;
	for(int i=0; i<POOLSIZE; i++){
		if(swap_table[i].sw_asid == NOPROC)
			return i;
	}
	return f++ % POOLSIZE;
}
