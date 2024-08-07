#include "./headers/vmSupport.h"

extern pcb_PTR current_process;
pcb_PTR ssi_pcb;
extern int process_count;
extern struct list_head receiveMessageQueue;

struct swap_t* swap_table[POOLSIZE];
pcb_PTR swapPCB;
int swapMutex = 1;

static int index = 0;

void initSwapStructs() {
    // swap table (RAMSTART + (32 * PAGESIZE)); 

    for (int i = 0; i < POOLSIZE; i++) {
        // ...
        swap_table[i]->sw_asid = NOPROC; // frame unoccupied
    }
}
/* TODO P and V  ? */


// The Pager
int TLB_exception_handler() {

    ssi_payload_t support_str_payload = {
        .service_code = GETSUPPORTPTR,
        .arg = NULL,
    };
    support_t* supStruct;

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&support_str_payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&supStruct), 0);

    state_t* excState = supStruct->sup_exceptState[0].cause;
    // if it's a TLB-Modification we treat it as a program trap
    if (excState == 1) {
        // if its in mutual exclusion release it (sendMessage)
        
        // TODO Section 9 in sysSupport.c
    } 
    // gain mutual exclusion over the swap table
    
    // missing page number
    unsigned int p = (excState->entry_hi & GETPAGENO) >> VPNSHIFT;

    int i = index;
    index = (index + 1) % POOLSIZE;
 
    // if frame i is occupied
    if (swap_table[i]->sw_asid != NOPROC) {
        // mark it as non valid means V bit is off
        swap_table[i]->sw_pte->pte_entryLO = swap_table[i]->sw_pte->pte_entryLO & 0xFFFFFEFF;
        uTLB_RefillHandler();

    }

    // ...

    LDST(&current_process->p_s);
}

/* Since reading and writing to each U-proc’s flash device is limited to supporting paging, this module should
also contain the function(s) for reading and writing flash devices.*/
