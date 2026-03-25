
#include <kernel.h>


/* TOS_IFDEF assn8 */
void null_process(PROCESS self, PARAM param)
{
    while (42);
    become_zombie();
}
/* TOS_ENDIF assn8 */


void init_null_process()
{
    /* TOS_IFDEF assn8 */
    create_process(null_process, 0, 0, "Null process");
    /* TOS_ENDIF assn8 */
}
