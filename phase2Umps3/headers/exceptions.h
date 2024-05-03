#ifndef EXCEPTIONS_H_INCLUDED
#define EXCEPTIONS_H_INCLUDED

#include <umps3/umps/libumps.h>
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/msg.h"
#include "interrupts.h"

void uTLB_RefillHandler(void);
void exceptionHandler(void);

pcb_PTR receiveMessage(pcb_PTR, unsigned int*);
int sendMessage(pcb_PTR, unsigned int*, pcb_PTR sender);

void syscallHandler(void);
void passUpOrDie(int except_type, state_t *exceptionState);

#endif
