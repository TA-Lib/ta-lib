#ifndef TA_MEMORY_H
#define TA_MEMORY_H

#include <stdio.h>

/* Interface description. */
#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

/* Compilation options */
#ifdef TA_DEBUG
   #define TA_MEM_LIST    /* Build internal list */
   #define TA_MEM_WHERE   /* Keep track of memory block source */
#else
   /* Some includes to get malloc,free,realloc */
   #include <stdlib.h> 
   #include <malloc.h>
#endif

/* Interface functions */
TA_RetCode      TA_MemInit   ( unsigned int memoryAlreadyInUse );
unsigned long   TA_MemUsed   ();
void            TA_MemDisplay( FILE *fp );

/* Interface functions to be accessed only through macros */
#if defined(TA_MEM_WHERE)
void      *TA_PrivAlloc(size_t, char *, int);
void      *TA_PrivRealloc(void *, size_t, char *, int);
void       TA_PrivFree(void *, char *, int);
char      *TA_PrivStrdup(char *, char *, int);
#else
void      *TA_PrivAlloc(size_t);
void      *TA_PrivRealloc(void *, size_t);
void       TA_PrivFree(void *);
char      *TA_PrivStrdup(char *);
#endif

/* Interface macros */
#if defined(TA_MEM_WHERE)
#define TA_Malloc(a)       TA_PrivAlloc((a),__FILE__,__LINE__)
#define TA_Realloc(a,b)    TA_PrivRealloc((a),(b),__FILE__,__LINE__)
#define TA_Free(a)         TA_PrivFree((a),__FILE__,__LINE__)
#define TA_Strdup(a)       TA_PrivStrdup((a),__FILE__,__LINE__)
#else
#define TA_Malloc(a)       malloc(a)
#define TA_Realloc(a,b)    realloc((a),(b))
#define TA_Free(a)         free(a)
#define TA_Strdup(a)       TA_PrivStrdup((a))
#endif

#define FREE_IF_NOT_NULL(x) { if((x)!=NULL) {TA_Free((void *)(x)); (x)=NULL;} }

/* A Typical function pointer for freeing memory. */
typedef TA_RetCode (*TA_FreeFuncPtr)( void *toBeFreed, void *opaqueData);

#endif


