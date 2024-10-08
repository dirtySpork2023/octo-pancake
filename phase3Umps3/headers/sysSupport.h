#ifndef SYSSUPPORT_H_INCLUDED
#define SYSSUPPORT_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"

#include "./initProc.h"

void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;

void generalExceptionHandler();
void usyscallHandler();
void programTrapsHandler();

#endif
