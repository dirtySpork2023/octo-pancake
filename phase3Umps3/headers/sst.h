#ifndef SST_H_INCLUDED
#define SST_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

#include "./initProc.h"
#include "./vmSupport.h"
#include "./sysSupport.h"

#define TERMSTATMASK 0xFF
#define TERM0ADDR 0x10000254
#define PRNT0ADDR 0x100001D4 // 0x1000.0054 + 3 * 0x80

void klog_print();
void klog_print_dec();
void klog_print_hex();
void breakPoint();
extern pcb_PTR current_process;

// system service thread
void SST();

// terminate SST and child after sending message to test
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
void terminate();

#endif
