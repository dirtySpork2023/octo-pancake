#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];
LIST_HEAD(pcbFree_h);
static int next_pid = 1;

// crea la lista di pcb
void initPcbs() {
    for(int i=0; i<MAXPROC; i++) {
		list_add(&pcbTable[i].p_list, &pcbFree_h);
	}
}

// aggiunge p alla lista pcbFree
void freePcb(pcb_t *p) {
    list_add_tail(&p->p_list, &pcbFree_h);
}

// ritorna un pcb dalla lista pcbFree resettando tutti i suoi campi
pcb_t *allocPcb() {
    if(list_empty(&pcbFree_h)) return NULL;
    else {
        pcb_PTR tmp = container_of(pcbFree_h.next, pcb_t, p_list);
        list_del(pcbFree_h.next);
        INIT_LIST_HEAD (&tmp->p_list);
        tmp->p_parent = NULL;
        INIT_LIST_HEAD(&tmp->p_child);
	    INIT_LIST_HEAD(&tmp->p_sib);
        // p_s risulta non inizializzato ?
        tmp->p_s.cause = 0;
        tmp->p_s.entry_hi = 0;
        for(int i = 0; i < STATE_GPR_LEN; i++)
            tmp->p_s.gpr[i] = 0;
        tmp->p_s.hi = 0;
        tmp->p_s.lo = 0;
        tmp->p_s.pc_epc = 0;
        tmp->p_s.status = 0;
        tmp->p_time = 0;
        INIT_LIST_HEAD (&tmp->msg_inbox);
        tmp->p_supportStruct = NULL;
        tmp->p_pid = next_pid++;
        return tmp;
    }
}

void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD (head);
}

int emptyProcQ(struct list_head *head) {
    // questa funzione risulta senza return ?
    return list_empty(head);
}

void insertProcQ(struct list_head *head, pcb_t *p) {
    list_add_tail(&p->p_list, head);
}

// ritorna il pcb in testa senza rimuoverlo
pcb_t *headProcQ(struct list_head *head) {
    return container_of(list_next(head), pcb_t, p_list);
}

// ritorna il pcb in testa togliendolo dalla lista
pcb_t *removeProcQ(struct list_head *head) {
    if(list_empty(head)) return NULL;
    else {
        pcb_PTR tmp = container_of(list_next(head), pcb_t, p_list);
        list_del(list_next(head));
        return tmp;
    }
}

// rimuove p dalla lista head
pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
    struct list_head* i;
    list_for_each(i, head) {
        if (i == &p->p_list) {
            list_del(i);
            return p;
        }
    } 
    return NULL;
}

// p_child è l'elemento sentinella della lista di p_sib

int emptyChild(pcb_t *p) {
	return list_empty( &p->p_child );
}

void insertChild(pcb_t *prnt, pcb_t *p) {
	p->p_parent = prnt;
	list_add_tail(&p->p_sib, &prnt->p_child);
}

// ritorna il figlio in testa togliendolo dalla lista
pcb_t *removeChild(pcb_t *p) {
	if(emptyChild(p)) return NULL;
	else{
		pcb_PTR child = container_of(p->p_child.next, pcb_t, p_sib);
		list_del(p->p_child.next);
		return child;
	}
}

// rimuove p dalla lista di siblings
pcb_t *outChild(pcb_t *p) {
	if( p->p_parent==NULL ) return NULL;
	else{
        p->p_parent = NULL;
		list_del(&p->p_sib);
		return p;
	}
}