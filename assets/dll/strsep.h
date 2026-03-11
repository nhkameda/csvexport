#ifndef _STRSEP_H
#define _STRSEP_H

char *strsep(char **stringp, const char *delim,int flags);

#define STRSEP_QUOTETOKEN   0x01  /* treat strings in double quotes as tokens */
#define STRSEP_SKIPWHITE    0x02  /* skip leading white-space */

#endif /* _STRSEP_H */
