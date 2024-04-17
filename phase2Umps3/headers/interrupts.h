#ifndef INTERRUPTS_H_INCLUDED
#define INTERRUPTS_H_INCLUDED

#include <umps3/umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/headers/pcb.h"
#include "scheduler.h"

void interruptHandler(int cause);
unsigned int getDeviceNumber (unsigned int interruptLine);

#endif
