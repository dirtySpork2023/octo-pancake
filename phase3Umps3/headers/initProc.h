#ifndef INITPROC_H_INCLUDED
#define INITPROC_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

#include "./sysSupport.h"
#include "./vmSupport.h"
#include "./sst.h"
#include "../../phase2Umps3/p2test.c"

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;

// instantiatorProcess
void test();

// SSI create process
pcb_PTR newProc(state_t *procState, support_t *procSupport);

// SSI kill self
void suicide(void);

void initSupportStruct(support_t *supportStruct, unsigned int asid);

#endif
