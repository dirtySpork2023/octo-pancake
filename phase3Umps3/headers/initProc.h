#ifndef INITPROC_H_INCLUDED
#define INITPROC_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

void newProc(state_t *procState, support_t *procSupport);

void test();

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;

#endif
