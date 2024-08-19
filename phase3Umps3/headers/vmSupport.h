#ifndef VMSUPPORT_H_INCLUDED
#define VMSUPPORT_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

#include "../../phase2Umps3/headers/exceptions.h"
#include "./sysSupport.h"

void initSwapStructs();
void P(pcb_PTR sender);
void V(pcb_PTR sender);
void pageFaultExceptionHandler();

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;
pcb_PTR ssi_pcb;
extern int process_count;
extern struct list_head receiveMessageQueue;


#endif
