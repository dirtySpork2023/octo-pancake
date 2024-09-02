#ifndef SST_H_INCLUDED
#define SST_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

#include "./initProc.h"
#include "./vmSupport.h"
#include "./sysSupport.h"

void klog_print();
void klog_print_dec();
void klog_print_hex();
void breakPoint();
extern pcb_PTR current_process;

// system service thread
void SST();

#endif
