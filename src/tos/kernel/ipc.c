
#include <kernel.h>


/* TOS_IFDEF assn5 */
PORT_DEF        port[MAX_PORTS];

PORT            next_free_port;
/* TOS_ENDIF assn5 */



PORT create_port()
{
    /* TOS_IFDEF assn5 */
    return create_new_port(active_proc);
    /* TOS_ENDIF assn5 */
}


PORT create_new_port(PROCESS owner)
{
    /* TOS_IFDEF assn5 */
    PORT            p;
    /* TOS_IFDEF assn7 */
    volatile int    flag;
    /* TOS_ENDIF assn7 */

    /* TOS_IFDEF assn7 */
    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    assert(owner->magic == MAGIC_PCB);
    if (next_free_port == NULL)
        panic("create_new_port(): PORT full");
    p = next_free_port;
    next_free_port = p->next;
    p->used = TRUE;
    p->magic = MAGIC_PORT;
    p->owner = owner;
    p->blocked_list_head = NULL;
    p->blocked_list_tail = NULL;
    p->open = TRUE;
    if (owner->first_port == NULL)
        p->next = NULL;
    else
        p->next = owner->first_port;
    owner->first_port = p;
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */

    return p;
    /* TOS_ENDIF assn5 */
}


/* TOS_IFDEF assn5 */
void add_to_send_blocked_list(PORT port, PROCESS proc)
{
    /* TOS_IFDEF assn7 */
    volatile int    flag;

    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    assert(port->magic == MAGIC_PORT);
    assert(proc->magic == MAGIC_PCB);
    if (port->blocked_list_head == NULL)
        port->blocked_list_head = proc;
    else
        port->blocked_list_tail->next_blocked = proc;
    port->blocked_list_tail = proc;
    proc->next_blocked = NULL;
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
}
/* TOS_ENDIF assn5 */


void open_port(PORT port)
{
    /* TOS_IFDEF assn5 */
    assert(port->magic == MAGIC_PORT);
    port->open = TRUE;
    /* TOS_ENDIF assn5 */
}



void close_port(PORT port)
{
    /* TOS_IFDEF assn5 */
    assert(port->magic == MAGIC_PORT);
    port->open = FALSE;
    /* TOS_ENDIF assn5 */
}


void send(PORT dest_port, void *data)
{
    /* TOS_IFDEF assn5 */
    PROCESS         dest;
    /* TOS_IFDEF assn7 */
    volatile int    flag;
    /* TOS_ENDIF assn7 */

    /* TOS_IFDEF assn7 */
    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    assert(dest_port->magic == MAGIC_PORT);
    dest = dest_port->owner;
    assert(dest->magic == MAGIC_PCB);

    if (dest_port->open && dest->state == STATE_RECEIVE_BLOCKED) {
        /* 
         * Receiver is receive blocked. We can deliver our message
         * immediately.
         */
        dest->param_proc = active_proc;
        dest->param_data = data;
        active_proc->state = STATE_REPLY_BLOCKED;
        add_ready_queue(dest);
    } else {
        /* 
         * Receiver is busy or the port is closed.
         * Get on the send blocked queue of the port.
         */
        add_to_send_blocked_list(dest_port, active_proc);
        active_proc->state = STATE_SEND_BLOCKED;
        active_proc->param_data = data;
    }
    active_proc->param_data = data;
    remove_ready_queue(active_proc);
    resign();
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    /* TOS_ENDIF assn5 */
}


void message(PORT dest_port, void *data)
{
    /* TOS_IFDEF assn5 */
    PROCESS         dest;
    /* TOS_IFDEF assn7 */
    volatile int    flag;
    /* TOS_ENDIF assn7 */

    /* TOS_IFDEF assn7 */
    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    assert(dest_port->magic == MAGIC_PORT);
    dest = dest_port->owner;
    assert(dest->magic == MAGIC_PCB);

    if (dest_port->open && dest->state == STATE_RECEIVE_BLOCKED) {
        dest->param_proc = active_proc;
        dest->param_data = data;
        add_ready_queue(dest);
    } else {
        /* 
         * Receiver is busy or the port is closed.
         * Get on the send blocked queue of the port.
         */
        add_to_send_blocked_list(dest_port, active_proc);
        remove_ready_queue(active_proc);
        active_proc->state = STATE_MESSAGE_BLOCKED;
        active_proc->param_data = data;
    }
    resign();
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    /* TOS_ENDIF assn5 */
}



void           *receive(PROCESS * sender)
{
    /* TOS_IFDEF assn5 */
    PROCESS         deliver_proc;
    PORT            port;
    void           *data;
    /* TOS_IFDEF assn7 */
    volatile int    flag;
    /* TOS_ENDIF assn7 */

    /* TOS_IFDEF assn7 */
    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    data = NULL;
    port = active_proc->first_port;
    if (port == NULL)
        panic("receive(): no port created for this process");
    while (port != NULL) {
        assert(port->magic == MAGIC_PORT);
        if (port->open && port->blocked_list_head != NULL)
            break;
        port = port->next;
    }

    if (port != NULL) {
        deliver_proc = port->blocked_list_head;
        assert(deliver_proc->magic == MAGIC_PCB);
        *sender = deliver_proc;
        data = deliver_proc->param_data;
        port->blocked_list_head = port->blocked_list_head->next_blocked;
        if (port->blocked_list_head == NULL)
            port->blocked_list_tail = NULL;

        if (deliver_proc->state == STATE_MESSAGE_BLOCKED) {
            add_ready_queue(deliver_proc);
            /* TOS_IFDEF assn7 */
            ENABLE_INTR(flag);
            /* TOS_ENDIF assn7 */
            return data;
        } else if (deliver_proc->state == STATE_SEND_BLOCKED) {
            deliver_proc->state = STATE_REPLY_BLOCKED;
            /* TOS_IFDEF assn7 */
            ENABLE_INTR(flag);
            /* TOS_ENDIF assn7 */
            return data;
        }
    }

    /* No messages pending */
    remove_ready_queue(active_proc);
    active_proc->param_data = data;
    active_proc->state = STATE_RECEIVE_BLOCKED;
    resign();
    *sender = active_proc->param_proc;
    data = active_proc->param_data;
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    return data;
    /* TOS_ENDIF assn5 */
}


void reply(PROCESS sender)
{
    /* TOS_IFDEF assn5 */
    /* TOS_IFDEF assn7 */
    volatile int    flag;

    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    if (sender->state != STATE_REPLY_BLOCKED)
        panic("reply(): Not reply blocked");
    add_ready_queue(sender);
    resign();
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    /* TOS_ENDIF assn5 */
}


void init_ipc()
{
    /* TOS_IFDEF assn5 */
    int             i;

    next_free_port = port;
    for (i = 0; i < MAX_PORTS - 1; i++) {
        port[i].used = FALSE;
        port[i].magic = MAGIC_PORT;
        port[i].next = &port[i + 1];
    }
    port[MAX_PORTS - 1].used = FALSE;
    port[MAX_PORTS - 1].magic = MAGIC_PORT;
    port[MAX_PORTS - 1].next = NULL;
    /* TOS_ENDIF assn5 */
}
