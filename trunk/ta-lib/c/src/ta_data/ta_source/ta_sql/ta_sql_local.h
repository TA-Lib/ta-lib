/* ta_sql_local.h
 * 
 * Prototypes of auxiliary functions, local to ta_sql driver
 */

#ifndef TA_SQL_LOCAL_H
#define TA_SQL_LOCAL_H

#if defined(__unix__)
/* remap to BSD 4.4 equivalents */
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

/* parse location URL; database[] has to be preallocated */
TA_RetCode TA_SQL_ParseLocation(const char location[],    /* IN  */
                                char scheme[],            /* OUT */
                                char host[],              /* OUT */
                                unsigned int *port,       /* OUT */
                                char database[]);         /* OUT */

/* Copy templateStr to new string on the heap replacing holderStr by valueStr */
char * TA_SQL_ExpandPlaceholders( const char *templateStr,  /* IN  */
                                  const char *holderStr,    /* IN  */
                                  const char *valueStr );   /* IN  */

#endif
