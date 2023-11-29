#include "./headers/msg.h"

static msg_t msgTable[MAXMESSAGES];
LIST_HEAD(msgFree_h);

void initMsgs() {
    for(int i=0; i<MAXPROC; i++) {
		list_add(&msgTable[i].m_list, &msgFree_h);
	}
}

void freeMsg(msg_t *m) {
    list_add_tail(&m->m_list, &msgFree_h);
}

msg_t *allocMsg() {
    if(list_empty(&msgFree_h)) return NULL;
    else {
        msg_PTR tmp = container_of(msgFree_h.next, msg_t, m_list);
        list_del(msgFree_h.next);
        INIT_LIST_HEAD(&tmp->m_list);
        tmp->m_sender = NULL;
        tmp->m_payload = 0;
        return tmp;
    }
}

void mkEmptyMessageQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

int emptyMessageQ(struct list_head *head) {
    return list_empty(head);
}

void insertMessage(struct list_head *head, msg_t *m) {
    list_add_tail(&m->m_list, head);
}

void pushMessage(struct list_head *head, msg_t *m) {
    list_add(&m->m_list, head);
}

msg_t *popMessage(struct list_head *head, pcb_t *p_ptr) {
    if(list_empty(head)) return NULL;
    if(p_ptr == NULL) return head->next;
    //...
    return NULL;
}

msg_t *headMessage(struct list_head *head) {
    return container_of(list_next(head), msg_t, m_list);
}
