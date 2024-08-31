#ifndef VMSUPPORT_H_INCLUDED
#define VMSUPPORT_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

#include "../../phase2Umps3/headers/exceptions.h"
#include "./sysSupport.h"

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;
//extern pcb_PTR ssi_pcb; => SSIADDRESS
//extern int process_count;
//extern struct list_head receiveMessageQueue;
extern pcb_PTR swap_pcb;

void initSwapStructs();
void cleanSwapTable(unsigned int asid);
void swapMutex();
void pageFaultExceptionHandler();
void flashDev(unsigned int cmd, unsigned int pageNo, unsigned int frameNo, unsigned int asid);


#endif
