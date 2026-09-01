#ifndef TA_MEMORY_H
#define TA_MEMORY_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#include <stdlib.h>

/* Interface macros */
#define TA_Malloc(a)       malloc(a)
#define TA_Realloc(a,b)    realloc((a),(b))
#define TA_Free(a)         free(a)

#define FREE_IF_NOT_NULL(x) { if((x)!=NULL) {TA_Free((void *)(x)); (x)=NULL;} }


/* ARRAY : Macros to manipulate arrays of value type.
 *
 * Using temporary array of double and integer are often needed for the
 * TA functions.
 *
 * These macros allow basic operations to alloc/copy/free array of value type.
 *
 * (Use ARRAY_REF and ARRAY_INT_REF for double/integer arrays).
 */
#define ARRAY_VTYPE_REF(type,name)             type *name
#define ARRAY_VTYPE_LOCAL(type,name,size)      type name[size]
#define ARRAY_VTYPE_ALLOC(type,name,size)      name = (type *)TA_Malloc( sizeof(type)*(size))
#define ARRAY_VTYPE_COPY(type,dest,src,size)   memcpy(dest,src,sizeof(type)*(size))
#define ARRAY_VTYPE_MEMMOVE(type,dest,destIdx,src,srcIdx,size) memmove( &dest[destIdx], &src[srcIdx], (size)*sizeof(type) )
#define ARRAY_VTYPE_FREE(type,name)            TA_Free(name)
#define ARRAY_VTYPE_FREE_COND(type,cond,name)  if( cond ){ TA_Free(name); }

/* ARRAY : Macros to manipulate arrays of double. */
#define ARRAY_REF(name)             ARRAY_VTYPE_REF(double,name)
#define ARRAY_LOCAL(name,size)      ARRAY_VTYPE_LOCAL(double,name,size)
#define ARRAY_ALLOC(name,size)      ARRAY_VTYPE_ALLOC(double,name,size)
#define ARRAY_COPY(dest,src,size)   ARRAY_VTYPE_COPY(double,dest,src,size)
#define ARRAY_MEMMOVE(dest,destIdx,src,srcIdx,size) ARRAY_VTYPE_MEMMOVE(double,dest,destIdx,src,srcIdx,size)
#define ARRAY_FREE(name)            ARRAY_VTYPE_FREE(double,name)
#define ARRAY_FREE_COND(cond,name)  ARRAY_VTYPE_FREE_COND(double,cond,name)

/* ARRAY : Macros to manipulate arrays of integer. */
#define ARRAY_INT_REF(name)             ARRAY_VTYPE_REF(int,name)
#define ARRAY_INT_LOCAL(name,size)      ARRAY_VTYPE_LOCAL(int,name,size)
#define ARRAY_INT_ALLOC(name,size)      ARRAY_VTYPE_ALLOC(int,name,size)
#define ARRAY_INT_COPY(dest,src,size)   ARRAY_VTYPE_COPY(int,dest,src,size)
#define ARRAY_INT_MEMMOVE(dest,destIdx,src,srcIdx,size) ARRAY_VTYPE_MEMMOVE(int,dest,destIdx,src,srcIdx,size)
#define ARRAY_INT_FREE(name)            ARRAY_VTYPE_FREE(int,name)
#define ARRAY_INT_FREE_COND(cond,name)  ARRAY_VTYPE_FREE_COND(int,cond,name)

/* ARRAY: Macros to manipulate arrays of mix type.
 * This is just a loop doing an element by element copy.
 */
#define ARRAY_MEMMOVEMIX_VAR int mmmixi, mmmixdestIdx, mmmixsrcIdx
#define ARRAY_MEMMOVEMIX(dest,destIdx,src,srcIdx,size) { \
            for( mmmixi=0, mmmixdestIdx=destIdx, mmmixsrcIdx=srcIdx; \
                mmmixi < size; \
                mmmixi++, mmmixdestIdx++, mmmixsrcIdx++ ) \
              { \
                  dest[mmmixdestIdx] = src[mmmixsrcIdx]; \
              } \
            }

/* Access to "Globals"
 *
 * The globals here just means that these variables are accessible from
 * all technical analysis functions.
 *
 * Depending of the language/platform, the globals might be in reality
 * a private member variable of an object...
 *
 * The second parameter of TA_GLOBALS_UNSTABLE_PERIOD is unused in C; it is
 * kept so the generated call sites stay stable across backends.
 */
#define TA_GLOBALS_UNSTABLE_PERIOD(x,y) (TA_Globals->unstablePeriod[x])
#define TA_GLOBALS_COMPATIBILITY        (TA_Globals->compatibility)



/* CIRCBUF : Circular Buffer Macros.
 *
 * A FIFO whose output rate equals its input rate: adding a value consumes the
 * slot it overwrites, so the oldest value must be READ BEFORE the new one is
 * written. `Id` can be any string, so several can live in one function.
 *
 *    CIRCBUF_PROLOG(Id,Type,Size)  declares `Id` (a Type*), `Id_Idx` (where to
 *                                  consume and add) and a Size-element array on
 *                                  the STACK -- keep Size reasonable.
 *    CIRCBUF_INIT(Id,Type,Size)    elects that stack array, or heap-allocates
 *                                  when the runtime Size does not fit it. Size
 *                                  here may be large. Pair with DESTROY on
 *                                  every exit path.
 *    CIRCBUF_NEXT(Id)              advance the index.
 *
 * The `_CLASS` variants and `CIRCBUF_REF` exist for a struct or class Type.
 */
#define CIRCBUF_PROLOG(Id,Type,Size) Type local_##Id[Size]; \
                                  int Id##_Idx; \
                                  Type *Id; \
                                  int maxIdx_##Id

/* Use this macro instead if the Type is a class or a struct. */
#define CIRCBUF_PROLOG_CLASS(Id,Type,Size) CIRCBUF_PROLOG(Id,Type,Size)

#define CIRCBUF_INIT(Id,Type,Size) \
   { \
      if( Size < 1 ) \
         return TA_INTERNAL_ERROR(137); \
      if( (int)Size > (int)(sizeof(local_##Id)/sizeof(Type)) ) \
      { \
         Id = TA_Malloc( sizeof(Type)*Size ); \
         if( !Id ) \
            return TA_ALLOC_ERR; \
      } \
      else \
         Id = &local_##Id[0]; \
      maxIdx_##Id = (Size-1); \
      Id##_Idx = 0; \
   }

#define CIRCBUF_INIT_CLASS(Id,Type,Size) CIRCBUF_INIT(Id,Type,Size)

#define CIRCBUF_INIT_LOCAL_ONLY(Id,Type) \
   { \
      Id = &local_##Id[0]; \
      maxIdx_##Id = (int)(sizeof(local_##Id)/sizeof(Type))-1; \
      Id##_Idx = 0; \
   }

#define CIRCBUF_DESTROY(Id) \
   { \
      if( Id != &local_##Id[0] ) \
         TA_Free( Id ); \
   }

/* Use this macro to access the member when Type is a class or a struct. */
#define CIRCBUF_REF(x) (x).

#define CIRCBUF_NEXT(Id) \
   { \
      Id##_Idx++; \
      if( Id##_Idx > maxIdx_##Id ) \
         Id##_Idx = 0; \
   }


#endif

