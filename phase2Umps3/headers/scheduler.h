#ifndef SCHEDULER_H_INCLUDED
#define SCHEDULER_H_INCLUDED

#include "../../headers/const.h"
#include "../../headers/types.h"
#include "./headers/kernel.h"
#include <umps/libumps.h>

void uTLB_RefillHandler();
void exceptionHandler();
void scheduler();

#endif