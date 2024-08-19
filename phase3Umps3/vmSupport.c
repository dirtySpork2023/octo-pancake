#include "./headers/vmSupport.h"

struct swap_t* swap_table[POOLSIZE];
pcb_PTR swapPCB;
int swapMutex;

static int index = 0;

void initSwapStructs() {
	// swap table (RAMSTART + (32 * PAGESIZE)); 
    // inizializzare ogni indice?
	swap_table[0] = (memaddr)(RAMSTART + (32 * PAGESIZE));

    swapMutex = 1;
	for (int i = 0; i < POOLSIZE; i++) {
		swap_table[i]->sw_asid = NOPROC; // frame unoccupied
	}
}

void P(pcb_PTR sender) {
    // ...

    SYSCALL(SENDMSG, (unsigned int)swapPCB, (unsigned int)0, 0);
}

void V(pcb_PTR sender) {
    // ...

    SYSCALL(SENDMSG, (unsigned int)swapPCB, (unsigned int)0, 0);
}


// exception codes 1-3 are passed up to here
// The Pager
// TLB entry found but invalid =>
// TLB_exception_handlvr
void pageFaultExceptionHandler() {

	ssi_payload_t support_str_payload = {
		.service_code = GETSUPPORTPTR,
		.arg = NULL,
	};
	support_t* supStruct;

	SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&support_str_payload, 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&supStruct), 0);

	
	state_t* excState = &supStruct->sup_exceptState[PGFAULTEXCEPT];
	// if it's a TLB-Modification we treat it as a program trap
	if (excState->cause == TLBMOD) {
		// if its in mutual exclusion release it (sendMessage)
		
		// TODO Program trap, Section 9 in sysSupport.c
	}
	
	// gain mutual exclusion over the swap table
	
	// missing page number
	unsigned int p = (excState->entry_hi & GETPAGENO) >> VPNSHIFT;

	static int i;
    i = index;
	index = (index + 1) % POOLSIZE;
 
	// if frame i is occupied
	if (swap_table[i]->sw_asid != NOPROC) {
		// operations done atomically by disabling interrupts
		//setSTATUS();
		
		// mark it as non valid means V bit is off
		swap_table[i]->sw_pte->pte_entryLO &= 0xFFFFFEFF;
		
		//update the TLB
        /* TODO after all other aspects of the Support Level are completed/debugged).
        Probe the TLB (TLBP) to see if the newly updated TLB entry is indeed cached in the TLB. If so (Index.P is 0), 
        rewrite (update) that entry (TLBWI) to match the entry in the Page Table */
        TLBCLR();

		//re-enable interrupts
		//setSTATUS();
		
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

        SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
		SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&response), 0);
        
        // if errors, treat as program trap
        if (response >= 4) {
            trapExcHandler();
        }
	}
    // read the content of cp backing store

    // update the swap pool table's entry i
    // swap_table[i]->sw_pte = ;
    swap_table[i]->sw_pageNo = p;
    swap_table[i]->sw_asid = current_process->p_supportStruct->sup_asid;

    // update the process' page table
    // current_process->p_supportStruct->sup_privatePgTbl[p].pte_entryLO = ;

    // update tlb
    TLBCLR(); // to modify when all is done

    // release mutual exclusion (TODO)
    SYSCALL(SENDMSG, (unsigned int)swapPCB, (unsigned int)0, 0);

    LDST(&current_process->p_s);
}

/* Since reading and writing to each U-proc’s flash device is limited to supporting paging, this module should
also contain the function(s) for reading and writing flash devices.*/
