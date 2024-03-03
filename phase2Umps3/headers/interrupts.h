#ifndef INTERRUPTS_H_INCLUDED
#define INTERRUPTS_H_INCLUDED

#include <umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/pcb.c"
#include "../main.c"

void interruptHandler(int cause);

#endif