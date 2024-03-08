#ifndef SSI_H_INCLUDED
#define SSI_H_INCLUDED

#include <umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/pcb.c"
#include "../main.c"

void initSSI();
void systemServiceInterface();

void createProcess(ssi_create_process_PTR, pcb_PTR);
void killProcess(pcb_PTR);
void doIO(ssi_do_io_PTR);
void getTime(pcb_PTR);
void waitForClock(pcb_PTR);
void getSupportStruct(pcb_PTR);
void getPID(pcb_PTR);

#endif