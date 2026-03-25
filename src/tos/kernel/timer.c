
#include <kernel.h>


PORT            timer_port;

/* TOS_IFDEF assn8 */
void timer_notifier(PROCESS self, PARAM param)
{
    while (42) {
        wait_for_interrupt(TIMER_IRQ);
        message(timer_port, 0);
    }
    become_zombie();
}

void timer_process(PROCESS self, PARAM param)
{
    int             ticks_left[MAX_PROCS];
    Timer_Message  *msg;
    PROCESS         sender;
    int             i;

    for (i = 0; i < MAX_PROCS; i++)
        ticks_left[i] = 0;

    create_process(timer_notifier, 7, 0, "Timer notifier");

    while (42) {
        msg = (Timer_Message *) receive(&sender);
        if (msg == NULL) {
            // The timer notifier sent us a message
            for (i = 0; i < MAX_PROCS; i++) {
                if (ticks_left[i] == 0)
                    continue;
                if (--ticks_left[i] == 0) {
                    // Wake up client
                    reply(&pcb[i]);
                }
            }
        } else {
            // A client sent us a message
            i = sender - pcb;
            ticks_left[i] = msg->num_of_ticks;
        }
    }
    become_zombie();
}
/* TOS_ENDIF assn8 */


void sleep(int ticks)
{
    /* TOS_IFDEF assn8 */
    Timer_Message   msg;
    msg.num_of_ticks = ticks;
    send(timer_port, &msg);
    /* TOS_ENDIF assn8 */
}


void init_timer()
{
    /* TOS_IFDEF assn8 */
    timer_port = create_process(timer_process, 6, 0, "Timer process");
    resign();
    /* TOS_ENDIF assn8 */
}
