#ifndef EXCEPTIONS_H_INCLUDED
#define EXCEPTIONS_H_INCLUDED

#include <umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/msg.h"

void uTLB_RefillHandler(void);
void exceptionHandler(void);
void syscallHandler(void);

int receiveMessage(pcb_PTR, unsigned int);
int sendMessage(pcb_PTR, unsigned int);

#endif