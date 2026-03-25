
#include <kernel.h>


int k_strlen(const char *str)
{
    /* TOS_IFDEF assn1 */
    int             l = 0;
    while (*str++ != '\0')
        ++l;
    return (l);
    /* TOS_ENDIF assn1 */
}

void           *k_memcpy(void *dst, const void *src, int len)
{
    /* TOS_IFDEF assn1 */
    char           *cdst = (char *) dst;
    char           *csrc = (char *) src;
    while (len > 0) {
        *cdst++ = *csrc++;
        len--;
    }
    return (dst);
    /* TOS_ENDIF assn1 */
}

int k_memcmp(const void *b1, const void *b2, int len)
{
    /* TOS_IFDEF assn1 */
    unsigned char  *c1 = (unsigned char *) b1;
    unsigned char  *c2 = (unsigned char *) b2;
    while (len > 0) {
        int             d = *c1++ - *c2++;
        if (d != 0)
            return (d);
        len--;
    }

    return (0);
    /* TOS_ENDIF assn1 */
}

void k_memset(const void *b, char fill, int len)
{
    /* TOS_IFDEF assn1 */
    unsigned char  *c = (unsigned char *) b;
    while (len > 0) {
        *c = fill;
        c++;
        len--;
    }
    /* TOS_ENDIF assn1 */
}
