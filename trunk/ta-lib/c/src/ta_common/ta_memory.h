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
#endif

/* Interface functions */
TA_RetCode      TA_MemInit   ( TA_Libc *libHandle,
                               unsigned int memoryAlreadyInUse );
unsigned long   TA_MemUsed   ( TA_Libc *libHandle );
void            TA_MemDisplay( TA_Libc *libHandle, FILE *fp );

/* Interface functions to be accessed only through macros */
#if defined(TA_MEM_WHERE)
void      *TA_PrivAlloc(TA_Libc *,size_t, char *, int);
void      *TA_PrivAllocCopy(TA_Libc *,const char *, size_t, char *, int);
void      *TA_PrivRealloc(TA_Libc *,void *, size_t, char *, int);
void       TA_PrivFree(TA_Libc *,void *, char *, int);
char      *TA_PrivStrdup(TA_Libc *,char *, char *, int);
#else
void      *TA_PrivAlloc(TA_Libc *,size_t);
void      *TA_PrivAllocCopy(TA_Libc *,const char *, size_t);
void      *TA_PrivRealloc(TA_Libc *,void *, size_t);
void       TA_PrivFree(TA_Libc *,void *);
char      *TA_PrivStrdup(TA_Libc *,char *);
#endif

/* Interface macros */
#if defined(TA_MEM_WHERE)
#define TA_Malloc(lib,a)       TA_PrivAlloc((lib),(a),__FILE__,__LINE__)
#define TA_MallocCopy(lib,a,b) TA_PrivAllocCopy((lib),(a),(b),__FILE__,__LINE__)
#define TA_Realloc(lib,a,b)    TA_PrivRealloc((lib),(a),(b),__FILE__,__LINE__)
#define TA_Free(lib,a)         TA_PrivFree((lib),(a),__FILE__,__LINE__)
#define TA_Strdup(lib,a)       TA_PrivStrdup((lib),(a),__FILE__,__LINE__)
#else
#define TA_Malloc(lib,a)       TA_PrivAlloc((lib),(a))
#define TA_MallocCopy(lib,a,b) TA_PrivAllocCopy((lib),(a),(b))
#define TA_Realloc(lib,a,b)    TA_PrivRealloc((lib),(a),(b))
#define TA_Free(lib,a)         TA_PrivFree((lib),(a))
#define TA_Strdup(lib,a)       TA_PrivStrdup((lib),(a))
#endif

#define FREE_IF_NOT_NULL(lib,x) { if((x)!=NULL) {TA_Free((lib),(void *)(x)); (x)=NULL;} }

/* A Typical function pointer for freeing memory. */
typedef TA_RetCode (*TA_FreeFuncPtr)( TA_Libc *libHandle, 
                                      void *toBeFreed,
                                      void *opaqueData);


#endif


