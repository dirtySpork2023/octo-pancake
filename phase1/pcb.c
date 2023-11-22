#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];
LIST_HEAD(pcbFree_h);
static int next_pid = 1;

void initPcbs() {
    for(int i=0; i<MAXPROC; i++) {
		list_add(&pcbTable[i].p_list, &pcbFree_h);
	}
}

void freePcb(pcb_t *p) {
    list_add_tail(&p->p_list, &pcbFree_h);
}

pcb_t *allocPcb() {
    if(list_empty(&pcbFree_h)) return NULL;
    else {
        pcb_PTR tmp = container_of(pcbFree_h.next, pcb_t, p_list);
        list_del(pcbFree_h.next);
        INIT_LIST_HEAD (&tmp->p_list);
        tmp->p_parent = NULL;
        INIT_LIST_HEAD(&tmp->p_child);
        tmp->p_parent = NULL;
        tmp->p_time = NULL;
        INIT_LIST_HEAD (&tmp->msg_inbox);
        tmp->p_supportStruct = NULL;
        tmp->p_pid = 0;
        return tmp;
    }
}

void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD (head);
}

int emptyProcQ(struct list_head *head) {
    return list_empty(head);
}

void insertProcQ(struct list_head *head, pcb_t *p) {
    list_add_tail(p, head);
}

pcb_t *headProcQ(struct list_head *head) {
    return list_next(head);
}

pcb_t *removeProcQ(struct list_head *head) {
    if(list_empty(head)) return NULL;
    else {
        pcb_PTR tmp = head;
        list_del(head);
        return tmp;
    }
}

pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
    struct list_head* pos;
    list_for_each(pos, head) {
        if (pos == p) {
            list_del(pos);
            return p;
        }
    } 
    return NULL;
}

int emptyChild(pcb_t *p) {
}

void insertChild(pcb_t *prnt, pcb_t *p) {
}

pcb_t *removeChild(pcb_t *p) {
}

pcb_t *outChild(pcb_t *p) {
}
