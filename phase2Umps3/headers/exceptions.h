#ifndef EXCEPTIONS_H_INCLUDED
#define EXCEPTIONS_H_INCLUDED

#include <umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/pcb.c"
#include "../../phase1/msg.c"
#include "../main.c"

void uTLB_RefillHandler();
void exceptionHandler(void);
void syscallHandler(void);

void receiveMessage(void);
void sendMessage(void);
void createProcess(void);

#endif