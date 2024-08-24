#ifndef SSI_H_INCLUDED
#define SSI_H_INCLUDED

#include <umps3/umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/headers/pcb.h"

// System Service Interface
void SSI();
void killProcess(pcb_PTR, pcb_PTR);

#endif
