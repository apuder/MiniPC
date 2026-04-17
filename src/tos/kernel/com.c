/* 
 * Internet ressources:
 * 
 * http://workforce.cup.edu/little/serial.html
 *
 * http://www.lammertbies.nl/comm/info/RS-232.html
 *
 */


#include <kernel.h>
#include <uart.h>

PORT            com_port;


void init_uart()
{
    uart2_set_div(CLK_FREQ / 2400.0 + 0.5);
    uart2_set_stop_bits(2);
}


/* TOS_IFDEF assn9 */

void com_reader_process(PROCESS self, PARAM param)
{
    PORT            reply_port;
    PROCESS         sender_proc;
    COM_Message    *msg;
    int             i;

    reply_port = (PORT) param;
    while (1) {
        msg = (COM_Message *) receive(&sender_proc);
        i = 0;
        while (i != msg->len_input_buffer) {
            wait_for_interrupt(UART2_IRQ);
            msg->input_buffer[i++] = uart2_getchar();
        }
        message(reply_port, &msg);
    }
    become_zombie();
}


void send_cmd_to_com(char *cmd)
{
    while (*cmd != '\0') {
        uart2_putchar(*cmd);
        cmd++;
    }
}


void com_process(PROCESS self, PARAM param)
{
    PORT            com_reader_port;
    PORT            com_writer_port;
    PROCESS         sender_proc;
    PROCESS         recv_proc;
    COM_Message    *msg;

    /* create a second port for receiving messages from COM reader process 
     */
    com_writer_port = create_new_port(self);

    /* create a port for COM reader process */
    com_reader_port = create_process(com_reader_process, 7,
                                     (PARAM) com_writer_port,
                                     "COM reader");

    while (42) {
        open_port(com_port);
        close_port(com_writer_port);
        msg = (COM_Message *) receive(&sender_proc);    // receive a
        // message from
        // user process
        message(com_reader_port, msg);
        send_cmd_to_com(msg->output_buffer);

        close_port(com_port);
        open_port(com_writer_port);
        receive(&recv_proc);    // receive a message from COM reader
        // process
        /* assert (recv_proc == com_reader_proc); */
        reply(sender_proc);
    }
    become_zombie();
}

/* TOS_ENDIF assn9 */

void init_com()
{
    /* TOS_IFDEF assn9 */
    init_uart();
    com_port = create_process(com_process, 6, 0, "COM process");
    resign();
    /* TOS_ENDIF assn9 */
}
