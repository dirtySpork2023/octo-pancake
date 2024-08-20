#ifndef SST_H_INCLUDED
#define SST_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase2Umps3/p2test.c" // for the constants used

#include "./initProc.h"
#include "./vmSupport.h"
#include "./sysSupport.h"

#define TERM0ADDR 0x10000254
#define PRNT0ADDR 0x100001D4 // 0x1000.0054 + 3 * 0x80

extern pcb_PTR current_process;

void SST();
unsigned int getTOD();
void terminate();
unsigned int writeString(sst_print_t* s, devregtr* base);

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;

// system service thread
void SST();

// number of microseconds since startup
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
unsigned int getTOD();

// terminate SST and child after sending message to test
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
void terminate();

// write string to printer (or terminal) of same number as child ASID
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
unsigned int writeString(sst_print_t* s, devregtr* base);

#endif
