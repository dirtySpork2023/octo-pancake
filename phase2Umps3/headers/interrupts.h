#ifndef INTERRUPTS_H_INCLUDED
#define INTERRUPTS_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/headers/pcb.h"
#include "scheduler.h"
#include "exceptions.h"

void interruptHandler(int cause);

void processorLocalTimer(); 
void intervalTimer();
void deviceInterrupt(int cause);

#endif
