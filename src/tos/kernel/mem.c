
#include <kernel.h>


void poke_b(MEM_ADDR addr, BYTE value)
{
    /* TOS_IFDEF assn2 */
    *((BYTE *) addr) = value;
    /* TOS_ENDIF assn2 */
}

void poke_w(MEM_ADDR addr, WORD value)
{
    /* TOS_IFDEF assn2 */
    *((WORD *) addr) = value;
    /* TOS_ENDIF assn2 */
}

void poke_l(MEM_ADDR addr, LONG value)
{
    /* TOS_IFDEF assn2 */
    *((LONG *) addr) = value;
    /* TOS_ENDIF assn2 */
}

BYTE peek_b(MEM_ADDR addr)
{
    /* TOS_IFDEF assn2 */
    return *((BYTE *) addr);
    /* TOS_ENDIF assn2 */
}

WORD peek_w(MEM_ADDR addr)
{
    /* TOS_IFDEF assn2 */
    return *((WORD *) addr);
    /* TOS_ENDIF assn2 */
}

LONG peek_l(MEM_ADDR addr)
{
    /* TOS_IFDEF assn2 */
    return *((LONG *) addr);
    /* TOS_ENDIF assn2 */
}
