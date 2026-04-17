#include <kernel.h>


/* TOS_IFDEF shell */
/* TOS_IFDEF train */


void train_set_switch(int number, char direction)
{
    COM_Message     msg;
    char            full_command[20];

    k_sprintf(full_command, "M%d%c\015", number, direction);
    msg.output_buffer = full_command;
    msg.len_input_buffer = 0;
    msg.input_buffer = NULL;
    send(com_port, &msg);
}

int train_probe(int number)
{
    COM_Message     msg;
    char            full_command[20];
    char            response[20];

    k_sprintf(full_command, "R\015");
    msg.output_buffer = full_command;
    msg.len_input_buffer = 0;
    msg.input_buffer = NULL;
    send(com_port, &msg);
    k_sprintf(full_command, "C%d\015", number);
    msg.output_buffer = full_command;
    msg.len_input_buffer = 3;
    msg.input_buffer = response;
    send(com_port, &msg);
    if (response[0] != '*' || response[2] != '\015') {
        return -1;
    }
    if (response[1] == '0') {
        return 0;
    } else if (response[1] == '1') {
        return 1;
    }
    return -1;
}

#if 0
void run_train_app(int window_id)
{
    static int      already_run = 0;

    if (already_run) {
        wm_print(window_id, "Train application already running.\n\n");
        return;
    }

    already_run = 1;
    init_train();
}
#endif
/* TOS_ENDIF train */

void shell_print_process_heading(int window_id)
{
    wm_print(window_id, "State           Active Prio Name\n");
    wm_print(window_id,
             "------------------------------------------------\n");
}

void shell_print_process_details(int window_id, PROCESS p)
{
    static const char *state[] = { "READY          ",
        "ZOMBIE         ",
        "SEND_BLOCKED   ",
        "REPLY_BLOCKED  ",
        "RECEIVE_BLOCKED",
        "MESSAGE_BLOCKED",
        "INTR_BLOCKED   "
    };

    if (!p->used) {
        wm_print(window_id, "PCB slot unused!\n");
        return;
    }
    /* State */
    wm_print(window_id, state[p->state]);
    /* Check for active_proc */
    if (p == active_proc) {
        wm_print(window_id, " *      ");
    } else {
        wm_print(window_id, "        ");
    }
    /* Priority */
    wm_print(window_id, "  %2d", p->priority);
    /* Name */
    wm_print(window_id, " %s\n", p->name);
}

void shell_ps(int window_id)
{
    PCB            *p = pcb;

    shell_print_process_heading(window_id);
    for (int i = 0; i < MAX_PROCS; i++, p++) {
        if (!p->used)
            continue;
        shell_print_process_details(window_id, p);
    }
}

void shell_top(int window_id)
{
    while (keyb_get_keystroke(window_id, FALSE) == 0) {
        wm_clear(window_id);
        shell_ps(window_id);
        sleep(10);
    }
}

void read_line(int window_id, char *buffer, int max_len)
{
    char            ch;
    int             i = 0;

    while (1) {
        ch = keyb_get_keystroke(window_id, TRUE);
        switch (ch) {
        case '\b':
            if (i == 0)
                continue;
            i--;
            break;
        case 13:
            buffer[i] = '\0';
            wm_print(window_id, "\n");
            return;
        default:
            if (i == max_len)
                break;
            buffer[i++] = ch;
            break;
        }
        char            str[2] = { ch, '\0' };
        wm_print(window_id, str);
    }
}


int is_command(char *s1, char *s2)
{
    while (*s1 == *s2 && *s2 != '\0') {
        s1++;
        s2++;
    }
    return *s2 == '\0' && (*s1 == '\0' || *s1 == ' ');
}

static char *skip_spaces(char *s)
{
    while (*s == ' ')
        s++;
    return s;
}

static char *skip_word(char *s)
{
    while (*s && *s != ' ')
        s++;
    return skip_spaces(s);
}

static char *parse_int(char *s, int *out)
{
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    *out = val;
    return s;
}

void process_command(int window_id, char *command)
{
    if (is_command(command, "ps")) {
        shell_ps(window_id);
        return;
    }

    if (is_command(command, "top")) {
        shell_top(window_id);
        return;
    }

    if (is_command(command, "clear")) {
        wm_clear(window_id);
        return;
    }

    if (is_command(command, "shell")) {
        start_shell();
        return;
    }

    if (is_command(command, "pong")) {
        start_pong();
        return;
    }

    /* TOS_IFDEF train */
    if (is_command(command, "train")) {
        char *args = skip_word(command);
        if (*args == '\0') {
            wm_print(window_id, "Running train app not yet supported.\n");
            //run_train_app(window_id);
        } else if (is_command(args, "switch")) {
            char *p = skip_word(args);
            int num = 0;
            p = parse_int(p, &num);
            p = skip_spaces(p);
            char dir = (*p == 'G' || *p == 'R') ? *p : '\0';
            if (dir == '\0') {
                wm_print(window_id, "Usage: train switch <number> G|R\n");
            } else {
                train_set_switch(num, dir);
            }
        } else if (is_command(args, "probe")) {
            char *p = skip_word(args);
            int num = 0;
            p = parse_int(p, &num);
            p = skip_spaces(p);
            if (*p != '\0') {
                wm_print(window_id, "Usage: train probe <number>\n");
            } else {
                switch (train_probe(num)) {
                    case -1:
                        wm_print(window_id, "Error probing track %d\n", num);
                        break;
                    case 0:
                        wm_print(window_id, "Track %d is clear\n", num);
                        break;
                    case 1:
                        wm_print(window_id, "Track %d is occupied\n", num);
                        break;
                }
            }
        } else {
            wm_print(window_id, "Usage: train [switch|probe]\n");
        }
        return;
    }

#if 0
    if (is_command(command, "go")) {
        set_train_speed("4");
        return;
    }

    if (is_command(command, "stop")) {
        set_train_speed("0");
        return;
    }

    if (is_command(command, "rev")) {
        set_train_speed("D");
        return;
    }
#endif
    /* TOS_ENDIF train */

    if (is_command(command, "help")) {
        wm_print(window_id, "Commands:\n");
        wm_print(window_id, "  - help   show this help\n");
        wm_print(window_id, "  - about  show credits\n");
        wm_print(window_id, "  - clear  clear window\n");
        wm_print(window_id, "  - shell  launch another shell\n");
        wm_print(window_id, "  - ps     show all processes\n");
        wm_print(window_id, "  - top    continuously show processes\n");
        wm_print(window_id, "  - pong   start PONG\n");
        /* TOS_IFDEF train */
#if 0
        wm_print(window_id, "  - go     make the train go\n");
        wm_print(window_id, "  - stop   make the train stop\n");
        wm_print(window_id, "  - rev    reverse train direction\n");
#endif
        wm_print(window_id, "  - train  start train application\n");
        wm_print(window_id, "  - train switch <n> G|R  set switch\n");
        wm_print(window_id, "\n");
        /* TOS_ENDIF train */
        return;
    }

    if (is_command(command, "about")) {
        wm_print(window_id, "TOS - A kludge by A. Puder\n\n");
        return;
    }

    /* Room for more commands! */
    wm_print(window_id, "Syntax error! Type 'help' for help.\n");
}


void print_prompt(int window_id)
{
    wm_print(window_id, "> ");
}


void shell_process(PROCESS self, PARAM param)
{
    char            buffer[80];

    int             window_id = wm_create(10, 3, 50, 17);

    wm_print(window_id, "TOS Shell\n");
    wm_print(window_id, "---------\n\n");

    while (1) {
        print_prompt(window_id);
        read_line(window_id, buffer, 45);
        process_command(window_id, buffer);
    }
    become_zombie();
}
/* TOS_ENDIF shell */


void start_shell()
{
    /* TOS_IFDEF shell */
    create_process(shell_process, 5, 0, "Shell Process");
    resign();
    /* TOS_ENDIF shell */
}
