/* TOS_IFDEF never */
/*!
\mainpage 
Welcome to the documentation for the Train Operating System for CSC720.
\n Throughout the semester, you will be implementing the core features of a functional operating system for a model train!
\n While doing so, you will get hands-on experience with many important concepts of modern operating systems, including among others:
\n
\li Processes
\li Preemptive multitasking
\li Task scheduling
\li Context switches
\li Inter-process communication
\li Interrupts
\li Device drivers
\n
\section first_section Functions You Will Implement
Throughout the course you will implement the core functions of TOS in the *.c files located in the tos/kernel directory.  
\n You will be able to test your implementations using the TOS test center, explained in the lecture slides.
\n \n
\section second_section Features Provided For You
Everything else is provided for you, and it is recommended that you look around all the files to become familiar with the features and overall organization of TOS.
\n Some important features that you will use extensively later in the course include: 
\n (1) a serial line communication simulator, with which you will practice I/O through the COM1 port. 
\n (2) a train simulator, which you will use to develop and test your final project.
*/

/*! 
\file kernel.h
\brief Header file for the TOS kernel.
\details This header file defines many variables, typedefs, structs, and functions that are used throughout the TOS API.
*/

/*! 
\file com.c
\brief Serial port driver and the COM service
*/
/*! 
\file dispatch.c
\brief Task scheduling
*/
/*! 
\file inout.c
\brief Reading and writing of bytes through the serial port
*/
/*! 
\file intr.c
\brief Interrupt handling and the interrupt descriptor table
*/
/*! 
\file ipc.c
\brief Inter-process communication
*/
/*! 
\file keyb.c
\brief The keyboard driver
*/
/*! 
\file main.c
\brief The entry point for TOS execution
*/
/*! 
\file malloc.c
\brief Dynamic memory management
*/
/*! 
\file mem.c
\brief Basic operations for memory access
*/
/*! 
\file null.c
\brief Implementation details of the null process
*/
/*! 
\file pong.c
\brief Implementation details of the pong game
*/
/*! 
\file process.c
\brief Management of TOS processes
*/
/*! 
\file stdlib.c
\brief Implementations of standard C library functions
*/
/*! 
\file timer.c
\brief The timer service
*/
/*! 
\file window.c
\brief The windowing system
*/
/*! 
\file wm.c
\brief Advanced window management and new features
*/
/* TOS_ENDIF never */

#ifndef __KERNEL__
#define __KERNEL__

#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

#define CLK_FREQ 84000000

#define TRUE   1
#define FALSE  0


#ifndef NULL
#define NULL   ((void *) 0)
#endif

#define boot_name "Boot process"

/* TOS_IFDEF never */
/*! 
\brief Typedef for true/false values, as an integer.
\details Assigns the name BOOL as an alternative name for the integer data type.  
*/
/* TOS_ENDIF never */
typedef int BOOL;


/*=====>>> stdlib.c <<<=====================================================*/

/* TOS_IFDEF never */
/*! 
\brief Calculates the length of a string.
\details Calculates the length of the string pointed to by str, excluding the null-terminating byte ('\0').
\param[in] str Pointer to an array of character constants.
\return The number of characters in the string pointed to by str.  
*/
/* TOS_ENDIF never */
int k_strlen(const char* str);

/* TOS_IFDEF never */
/*! 
\brief Copies a region of memory of a specified length.
\details Copies len bytes from memory region src to memory region dst. The source and destination memory regions must not overlap. 
\param[in] src A void pointer to the source memory region.
\param[in] len Integer number of bytes to copy.
\param[out] dst A void pointer to the destination memory region.
\sa k_memcmp(), k_memset()
*/
/* TOS_ENDIF never */
void* k_memcpy(void* dst, const void* src, int len);

/* TOS_IFDEF never */
/*! 
\brief Compares two memory areas of specified length. 
\details Compares the first len bytes of memory regions b1 and b2. 
If all the bytes in the two regions are matching, the function returns 0.
The function returns an integer less than zero or more than zero if the first len bytes of b1 are found to be less than, or greater than, the first len bytes of b2 respectively.  
\param[in] b1 A void pointer to a first memory region.
\param[in] b2 A void pointer to a second memory region.
\param[in] len The number of bytes of each memory region to be compared.
\return An integer indicating the results of the comparison.  
\sa k_memcpy(), k_memset(), k_strlen()  
*/
/* TOS_ENDIF never */
int k_memcmp(const void* b1, const void* b2, int len);

/* TOS_IFDEF never */
/*! 
\brief Fills a memory region with a constant byte.
\details Fills the first len bytes of the memory region pointed to by b with the character fill.
\param[in] fill The character with which to fill the first len bytes of the memory region.
\param[in] len The number of bytes of the memory region to be filled with the input character fill.
\param[out] b A void pointer to the memory region in which the first len bytes are filled with the character.
\sa k_memcpy(), k_memcmp(), k_strlen()  
*/
/* TOS_ENDIF never */
void k_memset(const void* b, char fill, int len);



/*=====>>> mem.c <<<========================================================*/

/* TOS_IFDEF never */
/*! 
\brief Typedef for memory addresses within the main memory used by TOS.  
\details The MEM_ADDR type, like all pointers in the TOS API, is 4 bytes.  
*/
/* TOS_ENDIF never */
typedef unsigned       MEM_ADDR;

/* TOS_IFDEF never */
/*! 
\brief Typedef for a single-byte data type.  
\details BYTE is another name in TOS for the unsigned character data type. 
\sa BYTE, WORD, LONG 
*/
/* TOS_ENDIF never */
typedef unsigned char  BYTE;

/* TOS_IFDEF never */
/*! 
\brief Typedef for an unsigned 2-byte data type.
\details WORD is another name in TOS for the unsigned short data type. 
\sa MEM_ADDR, WORD, LONG  
*/
/* TOS_ENDIF never */
typedef unsigned short WORD;

/* TOS_IFDEF never */
/*! 
\brief Typedef for an unsigned 4-byte data type. 
\details LONG is another name in TOS for the unsigned integer data type.  
\sa MEM_ADDR, BYTE, WORD
*/
/* TOS_ENDIF never */
typedef unsigned       LONG;

/* TOS_IFDEF never */
/*! 
\brief Writes 1 byte to memory at the specified memory address.
\details Writes value, a 1-byte item of data of type #BYTE, to the memory address specified by addr.
\param[in] addr A #MEM_ADDR, or memory address, to which the #BYTE is to be written.
\param[in] value A #BYTE representing the value which is to be written to memory.  
\sa peek_b(), BYTE, MEM_ADDR
*/
/* TOS_ENDIF never */
void poke_b(MEM_ADDR addr, BYTE value);

/* TOS_IFDEF never */
/*! 
\brief Writes 2 bytes to the memory region starting at the specified memory address.
\details Writes value, a 2-byte item of data of type #WORD, to the 2-byte memory region starting at address addr.
\param[in] addr A #MEM_ADDR, or memory address, to which the #WORD is to be written.
\param[in] value A #WORD representing the value which is to be written to memory.  
\sa poke_w(), WORD, MEM_ADDR
*/
/* TOS_ENDIF never */
void poke_w(MEM_ADDR addr, WORD value);

/* TOS_IFDEF never */
/*! 
\brief Writes 4 bytes to the memory region starting at the specified memory address.
\details Writes value, a 4-byte item of data of type #LONG, to the 4-byte memory region starting at address addr.
\param[in] addr A #MEM_ADDR, or memory address, to which the #LONG is to be written.
\param[in] value A #LONG representing the value which is to be written to memory.  
\sa poke_l(), LONG, MEM_ADDR  
*/
/* TOS_ENDIF never */
void poke_l(MEM_ADDR addr, LONG value);

/* TOS_IFDEF never */
/*! 
\brief Reads 1 byte from the specified memory address.
\details Reads and returns a #BYTE from the memory address addr.
\param[in] addr The memory address from which the 1-byte #BYTE value is to be read.
\return The #BYTE read from memory address addr.
\sa peek_b(), MEM_ADDR, BYTE
*/
/* TOS_ENDIF never */
BYTE peek_b(MEM_ADDR addr);

/* TOS_IFDEF never */
/*! 
\brief Reads 2 bytes from the memory region starting at the specified memory address.
\details Reads and returns a #WORD from the memory region starting at address addr.
\param[in] addr The memory address from which the 2-byte #WORD value is to be read.
\return The #WORD read from the 2-byte memory region starting at memory address addr.
\sa peek_w(), MEM_ADDR, WORD
*/
/* TOS_ENDIF never */
WORD peek_w(MEM_ADDR addr);

/* TOS_IFDEF never */
/*! 
\brief Reads 4 bytes from the memory region starting at the specified memory address.
\details Reads and returns a #LONG from the memory region starting at address addr.
\param[in] addr The memory address from which the 4-byte #LONG value is to be read.
\return The #LONG read from the 4-byte memory region starting at memory address addr.
\sa peek_l(), MEM_ADDR, LONG
*/
/* TOS_ENDIF never */
LONG peek_l(MEM_ADDR addr);


/*=====>>> window.c <<<=====================================================*/

/* TOS_IFDEF never */
/*! 
\struct WINDOW
\brief Contains the properties of a specific window.
\details The members of this struct are as follows:\n
<TABLE>
  <TR>  
    <td>int  x, int y</td>
    <td>the (x,y) coordinates of the upper left corner of the window</td>
  </TR>
  <TR>
    <td>int  width, int height</td>  
    <td> the width and height in pixels of the window</td>
  </TR>
  <TR>
    <td>int  cursor_x, int cursor_y</td> 
    <td> the (x,y) coordinates of the cursor location for the window</td>
  </TR>
  <TR>
    <td>char cursor_char</td>      
    <td>the cursor character, displayed at the cursor location</td>
  </TR>
</TABLE>
*/
/* TOS_ENDIF never */
typedef struct {
  int  x, y;
  int  width, height;
  int  cursor_x, cursor_y;
  char cursor_char;
} WINDOW;

/* TOS_IFDEF never */
/*! 
\hideinitializer
\brief Pointer to a WINDOW struct for a window that covers the entire screen.
\details The kernel_window is a pointer to a WINDOW struct whose members have the following values:
x=0, y=0, width=80, height=25, cursor_x=0, cursor_y=0, cursor_char=' '.
In the assignment WINDOW* kernel_window = &kernel_window_def shown above, kernel_window_def is a WINDOW with the above values.  
As a result, this variable kernel_window is initialized with a pointer to that WINDOW.
*/
/* TOS_ENDIF never */
extern WINDOW* kernel_window;

/* TOS_IFDEF never */
/*! 
\brief Implements formatted-print features of the wprintf() and kprintf() functions.
\details Parses the format specifiers and conversions provided as parameters to wprintf() and kprintf()
calls and populates a character buffer with an output string, correctly formatted in accordance with the 
behavior of the C function printf().  The functions wprintf() and kprintf() then print the correctly-formatted content 
to a window.  The full list of supported format specifiers and the number of arguments expected for each specifier
is documented in wprintf() and kprintf().  
Like printf(), this function can take variable argument lists: the data type va_list 
supports such lists and enables this function to act accordingly to each format specifier.
\param[in] buf Pointer to the first character in a buffer containing the data to be printed.
\param[in] fmt Pointer to a first character in a constant character buffer containing the format specifier.
\param[in] argp A va_list, which is a type that holds information about variable arguments.
\sa wprintf(), kprintf()
*/
/* TOS_ENDIF never */
int k_vsprintf(char *buf, const char *fmt, va_list argp);
#define vsprintf k_vsprintf
int k_sprintf(char *buf, const char *fmt, ...);
#define sprintf k_sprintf

/* TOS_IFDEF never */
/*! 
\brief Places the cursor at a specific position.
\details The position of the cursor is set to be (x,y) relative to the upper left corner
of the window.  The position must be within the boundaries of the window.  
\param[in] wnd Pointer to the WINDOW struct that specifies the window.
\param[in] x Integer x-coordinate within the window where the cursor will be placed.
\param[in] y Integer y-coordinate within the window where the cursor will be placed.
\sa remove_cursor(), WINDOW
*/
/* TOS_ENDIF never */
void move_cursor(WINDOW* wnd, int x, int y);

/* TOS_IFDEF never */
/*! 
\brief Removes the cursor from a window.
\details Removes the cursor by displaying a blank character at the cursor location specified
by cursor_x and cursor_y of this WINDOW.
\param[in] wnd Pointer to the WINDOW struct that specifies the window.  
\sa move_cursor(), WINDOW
*/
/* TOS_ENDIF never */
void remove_cursor(WINDOW* wnd);

/* TOS_IFDEF never */
/*! 
\brief Displays the cursor at the current cursor location.
\details Shows the cursor of the window by displaying cursor_char at the current cursor location, 
specified by (cursor_x, cursor_y).
\param[in] wnd Pointer to the WINDOW struct that specifies the window.
\sa remove_cursor(), WINDOW
*/
/* TOS_ENDIF never */
void show_cursor(WINDOW* wnd);

/* TOS_IFDEF never */
/*! 
\brief Clears the contents of the window.
\details The window content is erased and the cursor is placed at the top left corner of the empty window.
\param[in] wnd Pointer to the WINDOW struct that specifies the window.
\sa WINDOW
*/
/* TOS_ENDIF never */
void clear_window(WINDOW* wnd);

/* TOS_IFDEF never */
/*! 
\brief Displays a character at the cursor location, and advances the cursor appropriately.  
\details The indicated character is displayed at the cursor location, and the cursor is advanced to
the next location appropriately in the same manner as within a shell.  If necessary, the content of the window 
scrolls upward accordingly, also in the same manner as a shell.  
\param[in] wnd Pointer to the WINDOW struct that specifies the window.
\param[in] ch The character to be displayed at the current cursor location.
\sa output_string(), WINDOW
*/
/* TOS_ENDIF never */
void output_char(WINDOW* wnd, unsigned char ch);

/* TOS_IFDEF never */
/*! 
\brief Displays a string starting at the cursor location, and advances the cursor appropriately after the end of the string.  
\details The string (i.e., null-terminated array of constant characters) is displayed at the cursor location, and the cursor is advanced to
the next location after the string appropriately in the same manner as within a shell.  If necessary, the content of the window 
scrolls upward accordingly, also in the same manner as a shell.  The implementation of this function calls output_char() for each 
character in the string until the null terminator is reached.
\param[in] wnd Pointer to the WINDOW struct that specifies the window.
\param[in] str Pointer to the first element in a string that is to be displayed.
\sa output_char(), WINDOW
*/
/* TOS_ENDIF never */
void output_string(WINDOW* wnd, const char *str);

/* TOS_IFDEF never */
/*! 
\brief Prints content to a window in accordance with the specified format or conversion.    
\details  kprintf() and wprintf() are the TOS version of the C function printf(), allowing formatted printing or conversion 
and variable argument lists.  Example usages of wprintf() can be found in the lectures slides. This function is the same as
kprintf(), except that kprintf() prints to the #kernel_window.  
Supported printf features and conversions include %d (decimal), %u (unsigned), %x (hexadecimal), %X (hex with capital letters),
%o (octal), %c (character), %s (string), %%m.n (field width and precision), %%-m.n (left adjustment), 
 %0m.n (zero-padding), %%*.* (width and precision taken from arguments), and %b (binary conversion).  
The function vsprintf() parses the format specifier and optional arguments and creates a character buffer containing
the output string correctly formatted in accordance with how printf() behaves. 
\param[in] wnd Pointer to the WINDOW struct that specifies the window.
\param[in] fmt Pointer to the string to be printed and which may include format tags.
\param[in] ... Refers to zero or more optional arguments that specify the format(s).
\sa kprintf(), vsprintf(), output_string(), WINDOW
*/
/* TOS_ENDIF never */
void wprintf(WINDOW* wnd, const char* fmt, ...);

/* TOS_IFDEF never */
/*! 
\brief Prints content to the #kernel_window in accordance with the specified format or conversion.
\details  kprintf() and wprintf() are the TOS version of the C function printf(), allowing formatted printing or conversion 
and variable argument lists.  Example usages of kprintf() can be found in the lectures slides. 
Supported printf features and conversions include %d (decimal), %u (unsigned), %x (hexadecimal), %X (hex with capital letters),
%o (octal), %c (character), %s (string), %%m.n (field width and precision), %%-m.n (left adjustment), 
 %0m.n (zero-padding), %%*.* (width and precision taken from arguments), and %b (binary conversion).  
The function vsprintf() parses the format specifier and optional arguments and creates a character buffer containing
the output string correctly formatted in accordance with how printf() behaves.
This function is identical to calling wprintf(kernel_window, const char* fmt).
\param[in] fmt Pointer to the string to be printed and which may include format tags.
\param[in] ... Refers to zero or more optional arguments that specify the format(s).
\sa wprintf(), vsprintf(), output_string(), kernel_window
*/
/* TOS_ENDIF never */
void kprintf(const char* fmt, ...);


/*=====>>> process.c <<<====================================================*/

/* TOS_IFDEF never */
/*!
\brief Specifies the maximum number of TOS processes that can be created.  
\details Processes in TOS can be created, but are never deleted.  There is a \ref _PCB "PCB" entry for every TOS process.  These entries are never deleted; once a process's \ref _PCB "PCB" entry is created, it is never removed.  After a TOS process has accomplished its task, the state of the TOS process can be changed to #STATE_ZOMBIE to indicate that it will not run again.  
*/
/* TOS_ENDIF never */
#define MAX_PROCS		20

/* TOS_IFDEF never */
/*!
\brief Specifies the number of elements in the array which implements the #ready_queue.
\details Processes in TOS are given priorities when they are created.  Permitted priority levels are 0 through 7.  Each of these priority numbers corresponds to one of the elements in the #ready_queue array.  Each element in that array is a pointer to a TOS process which is at the head of a doubly-linked list of existing TOS processes at that priority level.  
*/
/* TOS_ENDIF never */
#define MAX_READY_QUEUES	8

/* TOS_IFDEF never */
/*!
\brief This state is assigned to processes that are eligible to run and are ready to use CPU cycles.
\details Only processes with this state can be on the #ready_queue.  If a process is in any other state, it is not eligible for CPU cycles and must be off the ready_queue.  
*/
/* TOS_ENDIF never */
#define STATE_READY             0

/* TOS_IFDEF never */
/*!
\brief This state is assigned to processes that are permanently off the #ready_queue and will never run again.
\details Once a process has completed its task and has no need to ever run again, it should be removed permanently off the ready queue.  Such processes are assigned this state, which is irreversible.   
*/
/* TOS_ENDIF never */
#define STATE_ZOMBIE            1

/* TOS_IFDEF never */
/*!
\brief This state is assigned to a process that has called send(), but the receiver process has not yet called receive().
\details In inter-process communication, if a TOS process calls send() but the receiver process has not yet called receive(), it means that the receiver is not ready to receive the message, and the sender process must wait until the receiver calls receive().  Until the receiver calls receive(), the sender process waits in this blocked state and is off the #ready_queue.  Unlike #STATE_MESSAGE_BLOCKED, a process in this state will not go back to #STATE_READY when the receiver calls receive(); rather, a process in this state becomes #STATE_REPLY_BLOCKED and subsequently awaits a reply from the receiver.
*/
/* TOS_ENDIF never */
#define STATE_SEND_BLOCKED      2

/* TOS_IFDEF never */
/*!
\brief This state is assigned to a sending process in the situation where the process has called send() and the receiver process has called receive() but not yet called reply().    
\details When the sender uses send() instead of message(), it is because the sender needs a reply from the receiver before the sender can be put back on the #ready_queue.  This state is used to indicate that the sender is awaiting a reply from the receiver process.  After the receiver executes reply(), the sender process goes from this state to #STATE_READY and is put back on the ready queue.
*/
/* TOS_ENDIF never */
#define STATE_REPLY_BLOCKED     3

/* TOS_IFDEF never */
/*!
\brief This state is assigned to a process that has called receive(), but there are no messages pending from any other process.  
\details In inter-process communication, if a TOS process calls receive() but no other process has sent it a message (either via send() or message()), then the process that called receive() enters this blocked state in order to await the sending of a message by another process.  While receive-blocked, the process is off the #ready_queue. Once another process sends a message via either send() or message() to this process while this process is receive-blocked, then this process goes from receive-blocked to #STATE_READY.
*/
/* TOS_ENDIF never */
#define STATE_RECEIVE_BLOCKED   4

/* TOS_IFDEF never */
/*!
\brief This state is assigned to a process that has called message(), and the receiver process has not yet called receive().  
\details In inter-process communication, if a TOS process calls message() but the receiver process has not yet called receive(), it means that the receiver is not ready to receive the message, and the sender process must wait until the receiver calls receive().  Until the receiver calls receive(), the sender process waits in this blocked state and is off the #ready_queue.  Unlike #STATE_SEND_BLOCKED, a process in this state does not expect a reply from the receiver, so that when the receiver calls receive(), a #STATE_MESSAGE_BLOCKED process can be changed to #STATE_READY and be put back on the ready queue.   
*/
/* TOS_ENDIF never */
#define STATE_MESSAGE_BLOCKED   5

/* TOS_IFDEF never */
/*!
\brief This state is assigned to a process that is waiting for an interrupt to occur.
\details TOS utilizes keyboard interrupts, timer interrupts, and COM1 interrupts to accomplish various functions.  A TOS process that is waiting for any of these interrupts is placed into this state and remains off the #ready_queue.  When the interrupt occurs, the process is changed to #STATE_READY and put back on the ready queue so that it can resume activity.
*/
/* TOS_ENDIF never */
#define STATE_INTR_BLOCKED      6

/* TOS_IFDEF never */
/*!
\brief A value used to verify that a data structure being accessed is a \ref _PCB "PCB".
\details "MAGIC" codes are used in TOS as a debugging and security feature in assert() statements.  \ref _PCB "PCB" structures store this value in a field.  When a process attempts to access a PCB, this field is used to verify that the data structure being accessed is in fact a PCB.  Failure of this condition stops execution of TOS.
*/
/* TOS_ENDIF never */
#define MAGIC_PCB 0x4321dcba

/* TOS_IFDEF never */
/*! 
\struct _PCB
\brief Holds the information associated with a TOS process.
\details This struct is used to store information about a TOS process.  Each element of the \ref pcb array is a pointer to one of these structs.  
The members of this struct are as follows:
<TABLE>
  <TR>  
    <td>unsigned magic</td>
    <td>A "magic" value used to verify that the record containing this value is indeed a PCB entry.  
         The value is given by #MAGIC_PCB.
    </td>
  </TR>
  <TR>
    <td>unsigned used</td>  
    <td>An integer value indicating whether this PCB entry in the \ref pcb array has been assigned to a process yet.
        Once assigned, this PCB entry is never freed up or re-assigned to a different process.  
    </td>
  </TR>
  <TR>
    <td>unsigned short priority</td> 
    <td>An integer from 0 to 7 corresponding to the priority level of this process.</td>
  </TR>
  <TR>
    <td>unsigned short state</td>      
    <td>An integer associated by "define" directives to #STATE_READY, #STATE_SEND_BLOCKED, #STATE_RECEIVE_BLOCKED, #STATE_MESSAGE_BLOCKED,
        #STATE_REPLY_BLOCKED, #STATE_INTR_BLOCKED, or #STATE_ZOMBIE. 
    </td>
  </TR>
  <TR>  
    <td>#MEM_ADDR esp</td>
    <td>The value of the stack pointer for this process.</td>
  </TR>
  <TR>
    <td>\ref PROCESS param_proc</td>  
    <td>A temporary storage location for the \ref PROCESS that was passed as an input to send() or message().</td>
  </TR>
  <TR>
    <td>void* param_data</td> 
    <td>A temporary storage location for the data that was passed as an input to send() or message().</td>
  </TR>
  <TR>
    <td>\ref _PORT_DEF "PORT" first_port</td>      
    <td>The first port in a singly-linked list of ports owned by this process.</td>
  </TR>
  <TR>
    <td>\ref PROCESS next_blocked</td>      
    <td>The next process in the send-blocked list of processes.</td>
  </TR>
  <TR>
    <td>\ref PROCESS next, \ref PROCESS prev</td>      
    <td>The next and previous processes, used for maintaining the doubly-linked list at each priority level of the #ready_queue.</td>
  </TR>
  <TR>
    <td>char* name</td>      
    <td>The name of this process, as a character array.</td>
  </TR>
</TABLE>
The capacity of the array \ref pcb is indicated by #MAX_PROCS to be 20.  (Note that PCB is actually a typedef for "struct _PCB", where the leading underscore in the struct's name indicates that the typedef PCB should be used when implementing the TOS API.)
\sa \ref pcb, \ref PROCESS
*/
/* TOS_ENDIF never */
struct _PORT_DEF;

/* TOS_IFDEF never */
/*!
\typedef PORT
\brief This typedef creates the alternative name PORT for a pointer to a _PORT_DEF struct.
\details Each process in TOS owns one (or possibly more) ports, used for inter-process communication.  
This typedef allows PORT to be used as an alternative name for the struct that holds information associated with a particular port of a TOS process.
Note that while each process can own multiple ports, a port can only be owned by exactly one process.
\sa create_process()
*/
/* TOS_ENDIF never */
typedef struct _PORT_DEF* PORT;

struct _PCB;

/* TOS_IFDEF never */
/*!
\typedef PCB
\brief A typedef that creates PCB as an alternative name for the data type of _PCB struct.
*/
/* TOS_ENDIF never */
typedef struct _PCB PCB;


/* TOS_IFDEF never */
/*!
\typedef PROCESS
\brief Typedef for a pointer to a \ref _PCB "PCB" struct.
\details The \ref pcb array holds a \ref _PCB "PCB" entry for each process.  
This typedef allows PROCESS to be used as an alternative name for a pointer to a \ref _PCB "PCB".
\sa pcb, \ref _PCB "PCB"
*/
/* TOS_ENDIF never */
typedef PCB* PROCESS;

struct _PCB {
    unsigned       magic;
    unsigned       used;
    unsigned short priority;
    unsigned short state;
    MEM_ADDR       esp;
    PROCESS        param_proc;
    void*          param_data;
    PORT           first_port;
    PROCESS        next_blocked;
    PROCESS        next;
    PROCESS        prev;
    char*          name;
};

/* TOS_IFDEF never */
/*!
\struct _PORT_DEF
\brief Holds information associated with a TOS process's port, used for inter-process communication.
\details Each process in TOS owns one or more ports.  While each process can own multiple ports, a port can only be owned by exactly one process. The maximum number of ports across all TOS processes is defined as #MAX_PORTS (i.e., #MAX_PROCS*2, equal to 40). \n\n
Note that PORT is a typedef for "struct _PORT_DEF*", where the leading underscores in the struct's name indicate that the typedef PORT should be used when implementing TOS. (The "_PORT_DEF" link in the table below and throughout this documentation will redirect to this page.) \n\n
The members of this struct are described below:

<TABLE>
  <TR>  
    <td>unsigned magic</td>
    <td>A "magic" value used to verify that the record being accessed is indeed a PORT entry.  
         The value is given by #MAGIC_PORT.
    </td>
  </TR>
  <TR>  
    <td>unsigned used</td>
    <td>An unsigned integer indicating whether the entry in the array of ports is being used or not.  The array of ports is initialized by init_ipc().</td>
  </TR>
  <TR>  
    <td>unsigned open</td>
    <td>An unsigned integer indicating whether the port is open or closed.</td>
  </TR>
  <TR>  
    <td>PROCESS owner</td>
    <td>The process which owns this port.  A port can have exactly one owner.</td>
  </TR>
  <TR>  
    <td>\ref  PROCESS blocked_list_head</td>
    <td>The first process on the send-blocked list of this port.</td>
  </TR>
  <TR>  
    <td>\ref PROCESS blocked_list_tail</td>
    <td>The last process on the send-blocked list of this port.  
        The tail of the list is maintained in order to efficiently add new processes to the end of the list.
    </td>
  </TR>
  <TR>  
    <td>struct _PORT_DEF *next</td>
    <td>The next port (if any) owned by the owner of this port.</td>
  </TR>
</TABLE>
\sa PROCESS, \ref _PCB "PCB", init_ipc()
*/
/* TOS_ENDIF never */
/* TOS_IFDEF never */
/*!
\brief An array of twenty \ref _PCB "PCB" structs.
\details Each element in this array is a pointer to the PCB data structure for one TOS process.
TOS processes are never removed from this array: when a process has completed its task, 
its state becomes #STATE_ZOMBIE and it remains in this array.
\sa create_process(), \ref _PCB "PCB"
*/
/* TOS_ENDIF never */
extern PCB pcb[];

/* TOS_IFDEF never */
/*!
\brief A typedef that creates the alternative name PARAM for an unsigned integer type.  
\details PARAM can be used to pass an optional parameter to create_process()
so that the process that calls create_process() can communicate the parameter to the 
new process.
\sa create_process()
*/
/* TOS_ENDIF never */
typedef unsigned PARAM;

/* TOS_IFDEF never */
/*! 
\brief Creates a new TOS process and returns the \ref _PORT_DEF "PORT" owned by the new process (or, before ports are covered, returns a NULL pointer).
\details When the running process calls this function, a new TOS process is created through the following steps:\n
(1) The next available PCB entry in the global pcb array is allocated;\n   
(2) the elements of this PCB entry are initialized (see the lecture slide on create_process for a full description);\n  
(3) an available 16kb stack frame for this process is allocated;\n  
(4) the newly-allocated stack frame is initialized (see the lecture slide describing how this function populates the stack);\n
(5) the stack pointer is saved to the PCB.esp field for this PCB entry;\n
(6) the new process is added to the ready queue (but note that resign() is not called, and that the new process does not start running immediately);\n
(7) return either a null pointer (before the class covers ports) or the \ref _PORT_DEF "PORT" of the new process.\n\n
Note that the implementation of create_process() will change slightly once interrupts are introduced, later in the course.  See the "Preemptive Multitasking" slide deck for more information at the appropriate time in the course.  
\param[in] new_proc Function pointer to a C function that takes a PROCESS and a PARAM as input arguments and returns void.  This function pointer new_proc 
serves as the entry point for the newly-created TOS process
\param[in] prio Integer specifying the priority of this new process 
\param[in] param A parameter to be passed in to the new process and placed on the new process's stack frame
\param[in] name Pointer to the first element of a character array that indicates the name of the new process
\return \ref _PORT_DEF "PORT" The port owned by the newly-created process.   
\sa \ref _PORT_DEF "PORT", PARAM, PROCESS, \ref _PCB "PCB", pcb
*/
/* TOS_ENDIF never */
PORT create_process(void (*new_proc) (PROCESS, PARAM),
		    int prio,
		    PARAM param,
		    char *proc_name);


#ifdef XXX
PROCESS fork();
#endif

/* TOS_IFDEF never */
/*!
\brief Prints the details of the specified TOS process
\details The name, state, and priority of the TOS process is printed, 
with a special character (e.g., an asterisk) indicating if the process is currently the running process.
\param[in] wnd Pointer to the WINDOW to which the process details should be printed
\param[in] p The process whose details should be printed.
\sa print_all_processes()
*/
/* TOS_ENDIF never */
void print_process(WINDOW* wnd, PROCESS p);

/* TOS_IFDEF never */
/*!
\brief Prints the details of all existing TOS processes
\details The name, state, and priority of each existing TOS process is printed, with a special character (e.g., an asterisk) indicating if the process is currently the running process.  See the lecture slides on TOS processes for an example of how the output may look like.
One possible implementation of this function calls print_process() for each of the processes in the \ref pcb array. 
\param[in] wnd Pointer to the WINDOW to which the process details should be printed
\sa print_process(), pcb, \ref _PCB "PCB"
*/
/* TOS_ENDIF never */
void print_all_processes(WINDOW* wnd);

/* TOS_IFDEF never */
/*!
\brief Initializes the global variables associated with TOS process creation
\details This function clears all the entries in the \ref pcb array; prepares the doubly-linked list of entries so that each array entry contains the address of the next available PCB slot; prepares the pcb entry for the boot process, and sets the global variable active_proc (used throughout TOS) to the first entry of the \ref pcb array.
\sa pcb, \ref _PCB "PCB", PROCESS
*/
/* TOS_ENDIF never */
void init_process();



/*=====>>> dispatch.c <<<===================================================*/

/* TOS_IFDEF never */
/*!
\brief The actively-running process. 
\details active_proc is a pointer to the PCB entry for the currently-running process, meaning that this process is #STATE_READY and is currently using the CPU.  
\sa \ref _PCB "PCB", pcb, PROCESS, resign()
*/
/* TOS_ENDIF never */
extern PROCESS active_proc;

/* TOS_IFDEF never */
/*!
\brief An array of pointers to PCBs for those processes that are ready to use CPU cycles.
\details The ready-queue contains pointers to the PCBs of those processes that are #STATE_READY, meaning that they are eligible for CPU cycles.  A process in any other state is not eligible to use the CPU, either because it has become a "zombie" process or it is in one of the blocked states.  Only one process can use the CPU at any given time, so the ready_queue is consulted by the dispatcher to determine which process should be the next to run. TOS uses priority-based round robin scheduling.  The ready queue is an 8-element array, with each element corresponding to one of the priority levels, 0-7.  Higher-priority processes are chosen to run before lower-priority processes.  Within the same priority level, a round-robin scheduling approach is used; to implement this approach, each PCB in the ready_queue maintains a doubly-linked list of processes, so that within a priority level, the dispatcher will return the next process in this priority level (or, if there is only one process at this priority level, the same process will be returned).  See the lecture slide called "Scheduling" for more information.  
\sa dispatcher(), \ref _PCB "PCB", ready_queue
*/
/* TOS_ENDIF never */
extern PCB* ready_queue[];

/* TOS_IFDEF never */
/*!
\brief Selects and returns the next process to run
\details Only processes that are on the ready queue are eligible to be selected.  TOS uses priority-based round robin scheduling: higher-priority processes are chosen to run before lower-priority processes.  Within the same priority level, a round-robin scheduling approach is used: the next process on the ready queue at that priority level is chosen (or, if the process is the only one at its priority level, it will be chosen).  It is assumed that there is always at least one process on the ready queue: to guarantee that this assumption holds, a null process of priority 0 is always on the ready queue and never becomes blocked under any circumstances. See the lecture slide called "Scheduling" for more information.
\sa active_proc, ready_queue, init_null_process()
*/
/* TOS_ENDIF never */
PROCESS dispatcher();

/* TOS_IFDEF never */
/*!
\brief Adds a process to the ready queue
\details This function changes the state of process proc to #STATE_READY and adds the process into the ready queue, at the tail of the doubly-linked list of processes maintained by the ready queue for each of the possibly priority levels 0 through 7.  For example, if proc is a process at priority 5, proc will be added to the ready queue at priority 5 by placing it at the tail of the doubly-linked list occupying priority 5 of the ready queue.  See the lecture slide "Maintaining the Ready Queue" in the Processes slide deck for more information.
The process to be added must be in #STATE_READY.  Processes in any other state are not eligible to use the CPU, so they should not be on the ready queue until their state becomes #STATE_READY.
\param[in] proc Process to be added to the ready queue
\sa ready_queue, remove_ready_queue, dispatcher()
*/
/* TOS_ENDIF never */
void add_ready_queue (PROCESS proc);

/* TOS_IFDEF never */
/*!
\brief Removes a process from the ready queue
\details When a process's state becomes anything other than #STATE_READY (for example, if a state becomes blocked or a zombie), it must be removed from the ready queue because it has become ineligible for CPU cycles.  This function removes that process from the ready queue and modifies the ready queue's doubly-linked list at the appropriate prioirity level so as to restore the doubly-linked list after removing the process from it.  For example, if priority 5 of the ready queue has the doubly-linked list A-B-C, and remove_ready_queue(B) is called, this function updates the doubly-linked list to A-C.  
\param[in] proc Process to be removed from the ready queue
\sa ready_queue, add_ready_queue, dispatcher()
*/
/* TOS_ENDIF never */
void remove_ready_queue (PROCESS proc);

/* TOS_IFDEF never */
/*!
\brief Changes the state of the calling process to #STATE_ZOMBIE.
\details Upon calling this function, the calling process becomes a "zombie", a permanent and irrevocable state in which the process is never again eligible for CPU cycles.   This function also removes the calling process from the ready_queue and initiates a context switch by calling resign() immediately afterward.  Once a process calls become_zombie(), it will never again be any other state other than #STATE_ZOMBIE and will never run again.  The "zombie" state is useful for identifying those processes that will never be active or run again (as opposed to processes that are in a blocked state, which can be restored to #STATE_READY at some future time).
\sa ready_queue, remove_ready_queue(), resign()
*/
/* TOS_ENDIF never */
void become_zombie();

/* TOS_IFDEF never */
/*!
\brief Implements a context switch in TOS, giving another process a chance to run.
\details This function does the following steps:   
(1) it saves the context of the current process pointed to by the global variable active_proc;  
(2) it calls dispatcher() and assigns the process returned by dispatcher() to active_proc;
(3) it restores the context of the newly-chosen process. 
See the "Context Switch" slides for much more information about the operation of resign().  
Also note that the implementation of resign() changes somewhat when interrupts are introduced later in the course.  See the "Preemptive Multitasking" slide deck for more information at the appropriate time in the course.
\sa dispatcher(), active_proc, resign()
*/
/* TOS_ENDIF never */
void resign();

/* TOS_IFDEF never */
/*!
\brief Initializes the global variables associated with the ready queue.
\details This function is called by kernel_main() to prepare the ready queue and associated global variables.  In one possible implementation, each entry of the ready queue is initialized with a null pointer; and the global variable ready_procs in dispatch.c is set to zero; and the calling process (the boot process) is added to the ready queue.  
\sa ready_queue, \ref _PCB "PCB", add_ready_queue()
*/
/* TOS_ENDIF never */
void init_dispatcher();


/*=====>>> null.c <<<=======================================================*/

/* TOS_IFDEF never */
/*!
\brief Creates the null process.
\details As TOS starts to run, this function is called in order to create a "null process" at priority level 0 which will always be on the ready queue.  No other process will ever have priority level 0, so that dispatcher() will never choose the null process unless it is the only process on the ready queue (i.e., all other processes are #STATE_ZOMBIE or in one of the blocked states).  The null process exists to guarantee that there is always at least one process on the ready queue.  The null process should never become any state other than #STATE_READY.  The C function that defines the null process is simply an infinite while loop, so that like all other TOS processes, the null process never returns to its caller in the traditional sense.  
\sa create_process(), ready_queue
*/
/* TOS_ENDIF never */
void init_null_process();


/*=====>>> ipc.c <<<========================================================*/

/* TOS_IFDEF never */
/*!
\brief Specifies the maximum number of \ref _PORT_DEF "PORT"s supported in TOS.
\details In the inter-process communication of TOS, messages are delivered to ports.  Each TOS process can have one or more ports, but each port can be owned by exactly one process.  This number is the size of the array that holds the \ref _PORT_DEF "PORT" entries.
\sa \ref _PORT_DEF "PORT"
*/
/* TOS_ENDIF never */
#define MAX_PORTS	(MAX_PROCS * 2)

/* TOS_IFDEF never */
/*!
\brief A value used to verify that a data structure being accessed is a \ref _PORT_DEF "PORT".
\details "MAGIC" codes are used in TOS as a debugging and security feature in assert() statements.  Each \ref _PORT_DEF "PORT" stores this value in its "magic" field.  When a process attempts to access a \ref _PORT_DEF "PORT", this field is used to verify that the data structure being accessed is in fact a \ref _PORT_DEF "PORT".  Failure of this condition stops execution of TOS.
*/
/* TOS_ENDIF never */
#define MAGIC_PORT  0x1234abcd

/* TOS_IFDEF never */
/*!
\typedef PORT_DEF
\brief A typedef for a data structure which describes a PORT.
\details This typedef creates the alternative name PORT_DEF for the data type struct _PORT_DEF.
\sa PORT
*/
/* TOS_ENDIF never */
typedef struct _PORT_DEF {
    unsigned  magic;
    unsigned  used;              /* Port slot used? */
    unsigned  open;              /* Port open? */
    PROCESS   owner;             /* Owner of this port */
    PROCESS   blocked_list_head; /* First local blocked process */
    PROCESS   blocked_list_tail; /* Last local blocked process */
    struct _PORT_DEF *next;            /* Next port */
} PORT_DEF;

/* TOS_IFDEF never */
/*!
\brief Creates a new port for the calling process.
\details This function creates and returns a new port for the running process active_proc.  The port is initially open.  Note that while a process can own multiple ports, each port can be owned by exactly one process.
\sa \ref _PORT_DEF "PORT", \ref _PCB "PCB", active_proc, create_new_port()
*/
/* TOS_ENDIF never */
PORT create_port();

/* TOS_IFDEF never */
/*!
\brief Creates a new port for the process indicated by the parameter proc
\details This function creates and returns a new port for the \ref _PCB "PROCESS" indicated by proc.  The port is initially open. Note that while a process can own multiple ports, each port can be owned by exactly one process.
\sa \ref _PORT_DEF "PORT", \ref _PCB "PCB", active_proc, create_port()
*/
/* TOS_ENDIF never */
PORT create_new_port (PROCESS proc);

/* TOS_IFDEF never */
/*!
\brief Opens a port.
\details Opens the \ref _PORT_DEF "PORT" indicated by the parameter port.  Only messages that are in an open port can be delivered to the receiver.  (Messages can still be sent to a closed port, but they will not be delivered to the receiver; instead, the messages are queued for later delivery.)
\sa close_port()
*/
/* TOS_ENDIF never */
void open_port (PORT port);

/* TOS_IFDEF never */
/*!
\brief Closes a port.
\details Closes the \ref _PORT_DEF "PORT" indicated by the parameter port.  Messages can still be sent to a closed port, but they will not be delivered to the receiver.  Instead, they are queued for delivery at some later time, after the port has been re-opened.  
\sa open_port()
*/
/* TOS_ENDIF never */
void close_port (PORT port);

/* TOS_IFDEF never */
/*!
\brief Sends a message to the \ref _PORT_DEF "PORT" of a receiver process, and a reply to the message is expected.
\details This function sends a synchronous message to the destination \ref _PORT_DEF "PORT" indicated by the parameter dest_port. The void pointer data is the content of the message.  If the receiver process has not yet called receive(), the sender becomes #STATE_SEND_BLOCKED and is placed on the send-blocked list until the receiver calls receive(), at which time the sender becomes #STATE_REPLY_BLOCKED. If the receiver process had already called receive(), then the message is delivered immediately to the receiver and the sender process becomes #STATE_REPLY_BLOCKED and awaits the receiver's reply.  Once the receiver process calls reply(), the sender process becomes #STATE_READY and can go back on the ready queue.   See the slides on inter-process communication for more details.   
\param[in] dest_port A \ref _PORT_DEF "PORT" owned by the receiver process, which must be open if the message is to be deliverable. 
\param[in] data A void pointer to the data contained in the message.
\sa \ref _PORT_DEF "PORT", message(), receive(), reply()
*/
/* TOS_ENDIF never */
void send (PORT dest_port, void* data);

/* TOS_IFDEF never */
/*!
\brief Sends a message to the \ref _PORT_DEF "PORT" of a receiver process, from which no reply is expected.
\details This function sends a synchronous message to the destination \ref _PORT_DEF "PORT" indicated by the parameter dest_port. The void pointer data is the content of the message.  If the receiver process has not yet called receive(), the sender becomes #STATE_MESSAGE_BLOCKED until the receiver calls receive(), after which time the sender is put back on the ready queue.  If the sender becomes blocked, it is placed on the send-blocked list of the receiver, then goes off the ready queue and calls resign().  If the receiver process had already called receive(), then the message can be delivered immediately and the sender does not become blocked.
\param[in] dest_port A \ref _PORT_DEF "PORT" owned by the receiver process, which must be open if the message is to be deliverable.  See the slides on inter-process communication for more details.  
\param[in] data A void pointer to the data contained in the message.
\sa \ref _PORT_DEF "PORT", send(), receive(), reply()
*/
/* TOS_ENDIF never */
void message (PORT dest_port, void* data);

/* TOS_IFDEF never */
/*!
\brief Receives a message.
\details If there is a message pending for this process (i.e., one of the open ports owned by this process contains a message from a sender process), this function receives that message and reacts accordingly, depending on whether the sender used send() or message().  If no message is pending for this process, or if the port is closed, the process becomes #STATE_RECEIVE_BLOCKED and calls resign().  See the inter-process communication lecture slides for more details.
\param[out] sender A pointer which is modified by the receive() function to point to the sender \ref PROCESS (as such, this is an output argument).
\return A void pointer passed by the sender process to the content of the message.     
\sa \ref _PORT_DEF "PORT", send(), message()
*/
/* TOS_ENDIF never */
void* receive (PROCESS* sender);

/* TOS_IFDEF never */
/*!
\brief Replies to a message.
\details This function causes the receiver process to reply to the sender process.  The receiver must have previously received a message from the sender and the sender must be #STATE_REPLY_BLOCKED.  The receiver puts the sender back on the ready queue and then calls resign() to give another process a chance to run.
\param[in] sender The \ref PROCESS which sent the message and is awaiting the reply.
\sa send(), message(), receive()
*/
/* TOS_ENDIF never */
void reply (PROCESS sender);

/* TOS_IFDEF never */
/*!
\brief Initializes the global variables associated with inter-process communication.
\details This function is called by kernel_main() to initialize the entries of an array of pointers to \ref _PORT_DEF "PORT" data structures, and to assign the array to a global variable.  Among other things, each element of the array contains a pointer to the next element of the array and a field indicating whether the array entry is being used by a process. See the "IPC" slide deck for more information about the setup of inter-process communication.
\sa \ref _PORT_DEF "PORT"
*/
/* TOS_ENDIF never */
void init_ipc();


/*=====>>> intr.c <<<=======================================================*/

/* TOS_IFDEF never */
/*!
\brief Assembly instructions that disable the interrupt subsystem of TOS to avoid race conditions.
\details When interrupts are enabled in TOS, a context switch can occur between any two machine instructions in response to an interrupt being raised.  This condition is a problem, because many TOS functions contain critical sections that can lead to race conditions if a context switch happens at an unfortunate time.  This problem is addressed by disabling interrupts while in the critical sections of these functions.
\sa ENABLE_INTR    
*/
/* TOS_ENDIF never */
#define DISABLE_INTR(save)                                                   \
  do {                                                                     \
    register unsigned __irq_mask asm("a0") = ~0u;                      \
    asm volatile (".word 0x0605650b"                                    \
            : "+r"(__irq_mask)                                     \
            :                                                      \
            : "memory");                                          \
    (save) = (volatile int)__irq_mask;                                   \
  } while (0)

/* TOS_IFDEF never */
/*!
\brief Assembly instructions that enable the interrupt subsystem of TOS.
\details Many TOS functions have critical sections that can lead to race conditions if a context switch happens at an unfortunate time.  To address this problem, the interrupt subsystem is temporarily disabled while a critical section of code is being executed.  These assembly instructions re-enable it once the critical section has been completed.
\sa DISABLE_INTR
*/
/* TOS_ENDIF never */
#define ENABLE_INTR(save)                                                    \
  do {                                                                     \
    if ((unsigned)(save) != ~0u) {                                       \
      register unsigned __irq_mask asm("a0") = (unsigned)(save);      \
      asm volatile (".word 0x0605650b"                                \
              : "+r"(__irq_mask)                                 \
              :                                                  \
              : "memory");                                      \
    }                                                                    \
  } while (0)

/* TOS_IFDEF never */
/*!
\brief Holds the data fields associated with an x86 interrupt.
\details This struct holds information associated with an IDT ("Interrupt Descriptor Table") entry. For each of the 256 interrupts in the x86, the Interrupt Descriptor Table contains an entry (i.e., an Interrupt Descriptor) that defines the 16-bit selector, attributes (16 bits combined), and 32-bit offset for that interrupt.  See the "Interrupts" lecture slides for more information about the members of this struct. 
\sa init_idt_entry(), init_interrupts()
*/
/* TOS_ENDIF never */
typedef struct 
{
    unsigned short offset_0_15;
    unsigned short selector;
    unsigned short dword_count : 5;
    unsigned short unused      : 3;
    unsigned short type        : 4;
    unsigned short dt          : 1;
    unsigned short dpl         : 2;
    unsigned short p           : 1;
    unsigned short offset_16_31;
} IDT;

/* TOS_IFDEF never */
/*!
\brief The value of the segment selector corresponding to the CODE segment in TOS.
*/
/* TOS_ENDIF never */
#define CODE_SELECTOR 0x8

/* TOS_IFDEF never */
/*!
\brief The value of the segment selector corresponding to the DATA segment in TOS.
*/
/* TOS_ENDIF never */
#define DATA_SELECTOR 0x10

/* TOS_IFDEF never */
/*!
\brief The number of elements in the Interrupt Descriptor Table ("IDT").
\details The IDT is an array of 256 elements, where each element is a descriptor for one x86-supported interrupt.  When the interrupt subsystem of TOS is initialized, this array is created and each of its elements is initialized with an entry describing the associated interrupt.  
\sa IDT_ENTRY_SIZE 
*/
/* TOS_ENDIF never */
#define MAX_INTERRUPTS 256

/* TOS_IFDEF never */
/*!
\brief The number of bytes of an entry in the Interrupt Descriptor Table ("IDT").
\details The IDT is a 256-element array, where each element is the descriptor for one interrupt supported by the x86.  Each descriptor is an 8-byte data structure with information about (1) the selector and (2) offset for the ISR that handles the interrupt, and (3) attributes of that interrupt.
\sa MAX_INTERRUPTS
*/
/* TOS_ENDIF never */
#define IDT_ENTRY_SIZE 8

/* TOS_IFDEF never */
/*!
\hideinitializer
\brief A global variable indicating whether interrupts are enabled (TRUE) or disabled (FALSE).
\details Some of the older TOS test cases will stop working once support for interrupts is implemented, because of changes made to the implementations of create_process() and resign() during the pre-emptive multitasking part of the course.  The interrupts_initialized variable is consulted to ensure that create_process() and resign() will function correctly whether interrupts are enabled or disabled.   The variable is initially set to FALSE.  As the assembly instructions associated with #DISABLE_INTR and #ENABLE_INTR are executed, this variable is updated accordingly.
\sa BOOL, init_interrupts()
*/
/* TOS_ENDIF never */
extern BOOL interrupts_initialized;

/* TOS_IFDEF never */
/*!
\brief Initializes the IDT entry for a single interrupt.
\details This function initializes the entry in the IDT for the interrupt specified by the integer intr_no.  The offset of the IDT entry is populated with the value of the function pointer to the ISR.  See the "Interrupts" lecture slides for more information about the IDT data structures.
\param[in] intr_no Integer number of the interrupt that this IDT entry describes.
\param[in] isr Function pointer to a C function that takes no arguments and returns void; this function pointer is the entry point for the interrupt service routine for the interrupt numbered intr_no.
\sa init_interrupts(), IDT
*/
/* TOS_ENDIF never */
void init_idt_entry (int intr_no, void (*isr) (void));

/* TOS_IFDEF never */
/*!
\brief Causes a process to wait until the specified interrupt occurs.
\details The process calling this function will become #STATE_INTR_BLOCKED until the interrupt identified by intr_no occurs.  When that interrupt occurs, the process is added back to the ready queue.  A process can wait for only one interrupt at a time, and only one process can be waiting for a particular interrupt.  The valid values for intr_no are #TIMER_IRQ and #UART_IRQ, corresponding to the timer and UART receive interrupts respectively.
\sa TIMER_IRQ, UART_IRQ, init_interrupts(), init_idt_entry()
*/
/* TOS_ENDIF never */
void wait_for_interrupt (int intr_no);

/* TOS_IFDEF never */
/*!
\brief Initializes the interrupt subsystem of TOS.
\details This function initializes each of the 256 entries in the interrupt descriptor table (by calling init_idt_entry() 256 times, once for each interrupt).  See the "Interrupts" lecture slides for more information on the IDT entries and the different types of interrupts.  This function also sets the global variable interrupts_initialized to true, and enables interrupts by executing the assembly instruction "sti". Note: in TOS, the three types of interrupts that can be handled are the keyboard interrupt, timer interrupt, and COM1 interrupt.  
\sa init_idt_entry(), IDT
*/
/* TOS_ENDIF never */
void init_interrupts ();


/*=====>>> timer.c <<<===================================================*/

/* TOS_IFDEF never */
/*!
\brief The number associated with a timer interrupt.
\details On the RISC core, external interrupts are represented as bits in the IRQ bitmap input. The machine timer source uses index 7.
*/
/* TOS_ENDIF never */
#define TIMER_IRQ   7

/* TOS_IFDEF never */
/*!
\brief The number associated with a UART receive interrupt.
\details External interrupts are represented as bits in the PicoRV32 IRQ bitmap input. UART receive-ready uses index 8.
*/
/* TOS_ENDIF never */
#define UART_IRQ    8

/* TOS_IFDEF never */
/*!
\brief The number associated with a second UART receive interrupt.
\details External interrupts are represented as bits in the PicoRV32 IRQ bitmap input. UART2 receive-ready uses index 9.
*/
/* TOS_ENDIF never */
#define UART2_IRQ   9

/* TOS_IFDEF never */
/*!
\brief The number associated with a PS/2 keyboard receive interrupt.
\details External interrupts are represented as bits in the PicoRV32 IRQ bitmap input. PS/2 keyboard key-press uses index 10.
*/
/* TOS_ENDIF never */
#define KEYB_IRQ   10

#define IRQ_MASK_ALL_DISABLED   (~0u)
#define IRQ_MASK_ALL_ENABLED  (~((1u << TIMER_IRQ) | (1u << UART_IRQ) | (1u << UART2_IRQ) | (1u << KEYB_IRQ)))

/* TOS_IFDEF never */
/*!
\brief The \ref _PORT_DEF "PORT" owned by the timer process.
\details To use the timer service, a client process sends a Timer_Message to this port indicating the number of timer ticks for which the client process wants to wait.  The timer notifier process also messages this port whenever a timer interrupt occurs.  See the "Timer" lecture slides for more information.
\sa \ref _Timer_Message "Timer_Message", sleep()
*/
/* TOS_ENDIF never */
extern PORT timer_port;

/* TOS_IFDEF never */
/*!
\struct _Timer_Message
\brief Contains the information of the message which is sent by a client process to the timer service.  
\details The only member of this struct is the number of timer ticks that the client process wants to sleep.  (Note that Timer_Message is a typedef for "struct _Timer_Message", where the leading underscore in the struct's name indicates that the typedef Timer_Message should be used to access this data structure when implementing TOS.)
\sa sleep()
*/
/* TOS_ENDIF never */
struct _Timer_Message 
{
    int num_of_ticks;
}; 

/* TOS_IFDEF never */
/*!
\typedef Timer_Message
\brief A typedef for a data structure which describes the contents of a timer message.  
\details This typedef specifies Timer_Message as an alternative name for the struct _Timer_Message data type.
\sa sleep()
*/
/* TOS_ENDIF never */
typedef struct _Timer_Message Timer_Message;

/* TOS_IFDEF never */
/*!
\brief Puts the calling process to sleep (off the ready queue) for a specified number of timer ticks.
\details This function is a wrapper that client processes use to create and send a \ref _Timer_Message "Timer_Message" to the timer process, indicating that the client process wishes to sleep for the number of timer ticks indicated by the argument num_of_ticks.  
\param[in] num_of_ticks Integer specifying the number of timer ticks that the calling process wants to sleep.
\sa \ref _Timer_Message "Timer_Message" 
*/
/* TOS_ENDIF never */
void sleep(int num_of_ticks);

/* TOS_IFDEF never */
/*!
\brief Initializes the timer process.
\details This function creates the timer process and the port, called #timer_port, which is owned by the timer process.  This function also calls resign() to initiate a context switch.  
\sa \ref _Timer_Message "Timer_Message", sleep()
*/
/* TOS_ENDIF never */
void init_timer();


/*=====>>> inout.c <<<======================================================*/

/* TOS_IFDEF never */
/*!
\brief Reads a byte from the I/O port designated by the argument port.
\details The port should not be confused with the ports used for inter-process communication.  COM1 can be accessed via I/O ports 0x3F8 through 0x3FF, with the base address 0x3F8 defined as #COM1_PORT.  
\param[in] port The I/O port from which the byte is to be read.    
\sa outportb()
*/
/* TOS_ENDIF never */
unsigned char inportb (unsigned short port);

/* TOS_IFDEF never */
/*!
\brief Writes the byte value to the I/O port designated by the argument port.
\details The port should not be confused with the ports used for inter-process communication. COM1 can be accessed via I/O ports 0x3F8 through 0x3FF, with the base address 0x3F8 defined as #COM1_PORT.  
\param[in] port The I/O port to which the byte is to be written.
\param[in] value The byte (as an unsigned character) to write to the I/O port.
\sa inportb()
*/
/* TOS_ENDIF never */
void outportb (unsigned short port, unsigned char value);


/*=====>>> com.c <<<=====================================================*/

/* TOS_IFDEF never */
/*!
\brief The number associated with a COM1 interrupt.
\details In TOS the interrupt controller is re-programmed so that the COM1 interrupt is mapped to interrupt number 0x64.  COM1 is the port used to communicate with the model train.
*/
/* TOS_ENDIF never */
#define COM1_IRQ    0x64

/* TOS_IFDEF never */
/*!
\brief The base address of the I/O ports associated with COM1.  
\details It is important to note that these I/O ports are not to be confused with a \ref _PORT_DEF "PORT" used by TOS processes for inter-process communication.
*/
/* TOS_ENDIF never */
#define COM1_PORT   0x3f8

/* TOS_IFDEF never */
/*!
\brief The number associated with a COM2 interrupt.
\details In TOS the interrupt controller is re-programmed so that the COM2 interrupt is mapped to interrupt number 0x63.  COM2 is used to communicate with the TOS Test Center.
*/
/* TOS_ENDIF never */
#define COM2_IRQ    0x63

/* TOS_IFDEF never */
/*!
\brief The base address of the I/O ports associated with COM2.
\details It is important to note that these I/O ports are not to be confused with the \ref _PORT_DEF "PORT" used by TOS processes for inter-process communication.  
*/
/* TOS_ENDIF never */
#define COM2_PORT   0x2f8

/* TOS_IFDEF never */
/*!
\brief A \ref _PORT_DEF "PORT" owned by the COM process to receive messages from user processes.
\details This global variable refers to the \ref _PORT_DEF "PORT" which is owned by the COM process and receives messages (of type \ref _COM_Message "COM_Message") from user processes that wish to send and/or receive bytes from the COM1 I/O port.  This global is initialized in init_com(). Note that the COM process owns a second port, at which messages from the COM Reader process are received.  See the "Device Drivers" lecture slides for more information about the COM Process and COM Reader Process.
\sa init_com(), \ref _COM_Message "COM_Message", \ref _PORT_DEF "PORT" 
*/
/* TOS_ENDIF never */
extern PORT com_port;

/* TOS_IFDEF never */
/*!
\struct _COM_Message
\brief Holds the contents of a COM message.
\details To read or write a byte from/to the I/O port COM1, a process sends a message to the COM process.  The contents of that message are contained in this struct.  (Note that COM_Message is a typedef for "struct _COM_Message", where the leading underscore in the struct's name indicates that the typedef COM_Message should be used to access this data structure when implementing TOS.) \n\n 
The members of this struct include the following:
<TABLE>
  <TR>  
    <td>char* output_buffer</td>
    <td>string to be output to the COM1 port</td>
  </TR>
  <TR>  
    <td>char* input_buffer</td>
    <td>buffer where input from the COM1 port will be stored</td>
  </TR>
  <TR>  
    <td>int len_input_buffer</td>
    <td>the number of bytes to be read from the COM1 port</td>
  </TR>
</TABLE> 
\sa com_port, #COM1_PORT
*/
/* TOS_ENDIF never */
struct _COM_Message 
{
    char* output_buffer;
    char* input_buffer;
    int   len_input_buffer;
};

/* TOS_IFDEF never */
/*!
\typedef COM_Message
\brief Typedef for the data structure that holds the contents of a COM message.
\details This typedef allows COM_Message to serve as an alternative name for the struct _COM_Message.  The underlying data structure holds the contents of the message received by the COM process.
\sa com_port
*/
/* TOS_ENDIF never */
typedef struct _COM_Message COM_Message;

/* TOS_IFDEF never */
/*!
\brief Initializes the COM subsystem of TOS.
\details This function initializes the UART ("Universal Asynchronous Receiver/Transmitter") that implements the RS232 protocol for serial line communication used by TOS. This function also creates the COM process by calling create_process(). The global variable \ref com_port is set to point to the \ref _PORT_DEF "PORT" (not to be confused with an I/O port) that is owned by the COM process.  
\sa \ref _COM_Message "COM_Message", com_port
*/
/* TOS_ENDIF never */
void init_com();


/*=====>>> keyb.c <<<====================================================*/

/* TOS_IFDEF never */
/*!
\brief The \ref _PORT_DEF "PORT" owned by the keyboard process.
\details Processes that need to get a keystroke from the keyboard process send messages to this port.  
\sa init_keyb(), \ref _Keyb_Message "Keyb_Message"
*/
/* TOS_ENDIF never */
extern PORT keyb_port;

/* TOS_IFDEF never */
/*!
\struct _Keyb_Message
\brief Holds the contents of a message to the keyboard process 
\details When a client process requests that a keystroke be directed to a specific window, the client process sends a message to the keyboard process.  The contents of that message are contained in this data structure, which includes:\n (1) the window identifier \n(2) a pointer to a character for the desired keystroke \n(3) a boolean value indicating whether the calling process should be blocked if no keystroke is available.  If so, then the client process is #STATE_REPLY_BLOCKED while it waits for a reply from the Keyboard Process; if not, then the function keyb_get_keystroke() will return 0 immediately if no keystroke is available, so the calling process does not become blocked.  \n\n
Note that Keyb_Message is a typedef for "struct _Keyb_Message", where the leading underscore in the struct's name indicates that the typedef Keyb_Message should be used to access this data structure when implementing TOS.
\sa keyb_port, keyb_get_keystroke()
*/
/* TOS_ENDIF never */
struct _Keyb_Message {
    int window_id;
    BOOL block;
    char* key_buffer;
};

/* TOS_IFDEF never */
/*!
\typedef Keyb_Message
\brief Typedef for the data structure that contains a message to the keyboard process.
\details This typedef creates the alternative name Keyb_Message for the struct _Keyb_Message.  The underlying data structure holds the contents of a message to the keyboard process, indicating that the requesting process is requesting the next available keystroke to be provided for a specific window. This message is sent by keyb_get_keystroke().
\sa keyb_get_keystroke(), 
*/
/* TOS_ENDIF never */
typedef struct _Keyb_Message Keyb_Message;


/* TOS_IFDEF never */
/*!
\brief Queries the keyboard process for the next keystroke.
\details This function enables a user process to request the next available keystroke to a specific window.  The keystroke is returned only when the window identified by window_id has the input focus.  If the argument block is set to TRUE, then the calling process will be blocked until a keystroke is available; if set to FALSE, then this function returns zero to the caller immediately so that it is not blocked.  See the slides on the Window Manager and TOS++ for more information.
\param[in] window_id The integer identifier of an existing window that was previously created via wm_create().
\param[in] block Boolean indicator of whether the caller should be blocked until a keystroke is received.
\sa wm_create(), keyb_port
*/
/* TOS_ENDIF never */
char keyb_get_keystroke(int window_id, BOOL block);

/* TOS_IFDEF never */
/*!
\brief Initializes the keyboard process.
\details This process creates the keyboard process and assigns the \ref _PORT_DEF "PORT" returned by the call to create_process() to the global variable keyb_port.  Note: this process must be called after the initialization of the Window Manager via init_wm(). 
\sa keyb_get_keystroke(), keyb_port
*/
/* TOS_ENDIF never */
void init_keyb();



/*=====>>> shell.c <<<===================================================*/

/* TOS_IFDEF never */
/*!
\brief Starts a new TOS shell window.
\details This function will be implemented as a part of the final project.
*/
/* TOS_ENDIF never */
void start_shell();

/*=====>>> train.c <<<===================================================*/

/* TOS_IFDEF never */
/*!
\brief Initializes a train process in TOS. 
\details This function will be implemented as a part of the final project.
*/
/* TOS_ENDIF never */
void init_train();

/* TOS_IFDEF never */
/*!
\brief Sets the speed of the model train.
\details This function will be implemented as a part of the final project.
*/
/* TOS_ENDIF never */
void set_train_speed(char* speed);


/*=====>>> pacman.c <<<==================================================*/

/* TOS_IFDEF never */
/// @cond
/* TOS_ENDIF never */
void init_pacman(WINDOW* wnd, int num_ghosts);

/* TOS_IFDEF never */
/// @endcond
/* TOS_ENDIF never */

/*=====>>> malloc.c <<<=====================================================*/

/* TOS_IFDEF never */
/// @cond
/* TOS_ENDIF never */
#define ONE_MB (1024 * 1024)
extern char _end;
#define SBRK_BEGIN ((unsigned char*) (&_end))
#define SBRK_END (SBRK_BEGIN + (3 * ONE_MB))

/* TOS_IFDEF never */
/// @endcond
/* TOS_ENDIF never */

/* TOS_IFDEF never */
/*! 
\brief Typedef for an integer, representing the size in bytes of the memory region being requested from malloc().
\details This typedef creates size_t as an alternative name for the integer data type.  The type size_t is the data type of the argument to malloc() specifying how many bytes of memory from the high memory region (with addresses in the range 1-8 MB) are being requested by a calling process.
\sa malloc()
*/
/* TOS_ENDIF never */
typedef int size_t;

/* TOS_IFDEF never */
/*!
\brief Dynamically allocates a continuous region of memory.   
\details TOS supports high memory in the region from 1 MB to 8 MB, which is managed by malloc().  A caller of malloc() is allocated a continuous region of high memory.  Malloc keeps track of fragmentation in high memory using its own data structure and finds a sufficiently-sized region of memory for the caller.  The allocated memory is not initialized (e.g., zeroed out) in any way.  If no sufficiently-sized region of memory is available, malloc() stops TOS with a failed assertion.
\param[in] size The size of the requested memory region.
\return A void pointer to the allocated memory region.
\sa free()
*/
/* TOS_ENDIF never */

void *malloc(size_t size);

/* TOS_IFDEF never */
/*!
\brief Frees a previously-allocated region of memory.
\details The parameter ptr must have been previously returned by malloc().  This function should be called only once for a given pointer, and memory pointed to by ptr should not be accessed anymore after a call to free().
\param[in] ptr A void pointer to the memory region previously allocated by a call to malloc()
\sa malloc()
*/
/* TOS_ENDIF never */
void free(void *ptr);


/*=====>>> wm.c <<<=====================================================*/

/* TOS_IFDEF never */
/*!
\brief Creates a new window via the Window Manager service.
\details Unlike the window implementation from early on in the course, this function can create a window that overlaps with other windows.  The Z-order of the windows is discussed in more detail near the end of the course.  
\param[in] x The x-axis coordinate of the upper left corner of the window.
\param[in] y The y-axis coordinate of the upper left corner of the window.
\param[in] width The width of the window.
\param[in] height The height of the window.
\return Integer identifier of the created window. 
\sa wm_print
*/
/* TOS_ENDIF never */
int wm_create(int x, int y, int width, int height);

/* TOS_IFDEF never */
/// @cond
/* TOS_ENDIF never */
char* wm_get_buffer(int window_id);
void wm_redraw_window(int window_id);

/* TOS_IFDEF never */
/// @endcond
/* TOS_ENDIF never */

/* TOS_IFDEF never */
/*!
\brief Clears the window and places the cursor in the top left corner of the window. 
\param[in] window_id The integer identifier of the window to be cleared.
\sa wm_print
*/
/* TOS_ENDIF never */
void wm_clear(int window_id);

/* TOS_IFDEF never */
/*!
\brief Sets the cursor to a given position in the window. 
\details This function also allows the cursor character to be changed.
\param[in] window_id The identifier of the window.
\param[in] x The x-axis coordinate of the new cursor location.
\param[in] y The y-axis coordinate of the new cursor location.
\param[in] ch The new cursor character.
\sa wm_clear
*/
/* TOS_ENDIF never */
void wm_set_cursor(int window_id, int x, int y, char ch);

/* TOS_IFDEF never */
/*!
\brief Primts a formatted string to the window.
\details This function accepts strings formatted in the same way as the C printf() function.  Consult the documentation on wprintf() for more details about the available formats.
\param[in] window_id The identifier of the window.
\param[in] fmt Pointer to the string to be printed and which may include format tags.
\param[in] ... Refers to zero or more optional arguments that specify the format(s). 
\sa wprintf
*/
/* TOS_ENDIF never */
void wm_print(int window_id, const char* fmt, ...);

/* TOS_IFDEF never */
/// @cond
/* TOS_ENDIF never */
int wm_change_focus();
int wm_current_focus();
void wm_move_left(int window_id);
void wm_move_right(int window_id);
void wm_move_up(int window_id);
void wm_move_down(int window_id);
/* TOS_IFDEF never */
/// @endcond
/* TOS_ENDIF never */

/* TOS_IFDEF never */
/*!
\brief Initializes the Window Manager service.
\details This function is called once from kernel_main() and creates the Window Manager process.
*/
/* TOS_ENDIF never */
void init_wm();


/*=====>>> pong.c <<<=====================================================*/

/* TOS_IFDEF never */
/*!
\brief Starts a new game of pong.
\details In the shell application to be implemented as part of the final project, one of the available shell commands will be to start a new game of pong.
\sa start_shell()
*/
/* TOS_ENDIF never */
void start_pong();

/* TOS_IFDEF never */
// Files must be commented in order to be detectable with EXTRACT_ALL = NO specified in the Doxyfile.

/*! \file kernel/com.c  */
/*! \file kernel/dispatch.c */
/*! \file kernel/inout.c */
/*! \file kernel/intr.c */
/*! \file kernel/ipc.c */
/*! \file kernel/keyb.c */
/*! \file kernel/malloc.c */
/*! \file kernel/mem.c */
/*! \file kernel/main.c */
/*! \fn kernel_main
\brief The entry point of TOS unless running in test mode.
\details At the end of the boot sequence, TOS starts executing at this function.  
The responsibilities of this function are to turn off the VGA hardware cursor, to initialize the various subsystems of TOS, and to start a shell so that the user can enter commands.  Lastly, this function causes the boot process to enter the state #STATE_ZOMBIE.\n
You compile TOS using either "make" or "make tests".  The latter will build a testing kernel, and when TOS runs, the "kernel_main" function in the test directory, rather than this function, will execute.   
*/
/*! \file kernel/null.c */
/*! \file kernel/pong.c */
/*! \file kernel/process.c */
/*! \file kernel/shell.c */
/*! \file kernel/stdlib.c */
/*! \file kernel/timer.c */
/*! \file kernel/train.c */
/*! \file kernel/window.c */
/*! \file kernel/wm.c */

/* TOS_ENDIF never */

#endif
