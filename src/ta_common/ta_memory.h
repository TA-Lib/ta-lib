#ifndef TA_MEMORY_H
#define TA_MEMORY_H

#if !defined( _MANAGED )
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

   /* Interface functions (never call these directly. Use the macros instead) */
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
#endif /* !defined(_MANAGED) */

/* ARRAY : Macros to manipulate arrays of double.
 *
 * Using temporary array of double is often needed for the TA functions
 * and these macros allow basic operations to alloc/copy/free these.
 *
 * The macros offer the advantage to work in C/C++ and managed C++.
 */
#if defined( _MANAGED )
   #define ARRAY_REF(name)             double name __gc []
   #define ARRAY_ALLOC(name,size)      name = new double __gc [size]
   #define ARRAY_COPY(dest,src,size)   src->CopyTo(dest,0)
   #define ARRAY_MEMMOVE(dest,destIdx,src,srcIdx,size) Array::Copy( src, srcIdx, dest, destIdx, size )
   #define ARRAY_FREE(name)
   #define ARRAY_FREE_COND(cond,name)
#else
   #define ARRAY_REF(name)             double *name
   #define ARRAY_ALLOC(name,size)      name = (double *)TA_Malloc( sizeof(double)*(size))
   #define ARRAY_COPY(dest,src,size)   memcpy(dest,src,sizeof(double)*(size))
   #define ARRAY_MEMMOVE(dest,destIdx,src,srcIdx,size) memmove( &dest[destIdx], &src[srcIdx], (size)*sizeof(double) )
   #define ARRAY_FREE(name)            TA_Free(name)
   #define ARRAY_FREE_COND(cond,name)  if( cond ){ TA_Free(name); }
#endif

/* CBUF : Circular Buffer Macros.
 *
 * The CBUF is like a FIFO buffer (First In - First Out), except
 * that the rate of data coming out is the same as the rate of
 * data coming in (for simplification and speed optimization).
 * In other word, when you add one new value, you must also consume
 * one value (if not consume, the value is lost).
 *
 * The CBUF size is unlimited, so it will automatically allocate and
 * de-allocate memory as needed. When small enough, CBUF will instead
 * use a buffer "allocated" on the stack (automatic variable).
 * 
 * Multiple CBUF can be used within the same function. To make that
 * possible the first parameter of the MACRO is an "Id" that can be
 * any string.
 *
 * The macros offer the advantage to work in C/C++ and managed C++.
 * 
 * CIRCBUF_PROLOG(Id,Type,Size);
 *          Will declare all the needed variables. 2 variables are
 *          important to the CBUF user: 
 *                 1) 'Id' will be a ptr of the specified Type.
 *                 2) 'Id'_Idx indicates from where to consume and 
 *                     to add the data.
 *
 *          Important: You must consume the oldest data before
 *                     setting the new data!
 *
 *          The Size must be reasonable since it might "allocate"
 *          an array of this size on the stack (each element are 'Type').
 *
 * CIRCBUF_CONSTRUCT(Id,Type,Size);
 *         Must be called prior to use the remaining macros. Must be
 *         followed by CIRCBUF_DESTROY when leaving the function.
 *         The Size here can be large. If the static Size specified
 *         with CIRCBUF_PROLOG is not sufficient, this MACRO will
 *         allocate a new buffer from the Heap.
 *
 * CIRCBUF_DESTROY(Id,Size);
 *         Must be call prior to leave the function.
 *
 * CIRCBUF_NEXT(Id);
 *         Move forward the indexes.
 *
 * Example:
 *     TA_RetCode MyFunc( int size )
 *     {
 *        CIRCBUF_PROLOG(MyBuf,int,4);
 *        int i, value;
 *        ...
 *        CIRCBUF_CONSTRUCT(MyBuf,int,size);
 *        ...
 *        // 1st Loop: Fill MyBuf with initial values
 *        //           (must be done).
 *        value = 0;
 *        for( i=0; i < size; i++ )
 *        {
 *           // Set the data
 *           MyBuf[MyBuf_Idx] = value++;
 *           CIRCBUF_NEXT(MyBuf);
 *        }
 *
 *        // 2nd Loop: Get and Add subsequent values
 *        //           in MyBuf (optional)
 *        for( i=0; i < 3; i++ )
 *        {
 *           // Consume the data (must be done first)
 *           printf( "%d ", MyBuf[MyBuf_Idx] );
 *
 *           // Set the new data (must be done second)
 *           MyBuf[MyBuf_Idx] = value++;
 *
 *           // Move the index forward
 *           CIRCBUF_NEXT(MyBuf);
 *        }
 *
 *        // 3rd Loop: Empty MyBuf (optional)
 *        for( i=0; i < size; i++ )
 *        {
 *           printf( "%d ", MyBuf[MyBuf_Idx] );
 *           CIRCBUF_NEXT(MyBuf);
 *        }
 *
 *        CIRCBUF_DESTROY(MyBuf);
 *        return TA_SUCCESS;
 *     }
 *
 *
 * A call to MyFunc(5) will output:
 *    0 1 2 3 4 5 6 7
 *
 * The value 0 to 4 are added by the 1st loop.
 * The value 5 to 7 are added by the 2nd loop.
 *
 * The value 0 to 2 are displayed by the 2nd loop.
 * The value 3 to 7 are displayed by the 3rd loop.
 *
 * Because the size 5 is greater than the 
 * value provided in CIRCBUF_PROLOG, a buffer will
 * be dynamically allocated (and freed).
 */
#if defined( _MANAGED )

#define CIRCBUF_PROLOG(Id,Type,Size) int Id##_Idx = 0; \
                                     Type Id __gc []; \
                                     int maxIdx_##Id = (Size-1)

#define CIRCBUF_INIT(Id,Type,Size) \
   { \
      if( Size <= 0 ) \
         return TA_ALLOC_ERR; \
      Id = new Type __gc [Size]; \
      if( !Id ) \
         return TA_ALLOC_ERR; \
      maxIdx_##Id = (Size-1); \
   }

#define CIRCBUF_INIT_LOCAL_ONLY(Id,Type) \
   { \
      Id = new Type __gc [maxIdx_##Id+1]; \
      if( !Id ) \
         return TA_ALLOC_ERR; \
   }

#define CIRCBUF_DESTROY(Id)

#else

#define CIRCBUF_PROLOG(Id,Type,Size) Type local_##Id[Size]; \
                                  int Id##_Idx; \
                                  Type *Id; \
                                  int maxIdx_##Id

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
#endif

#define CIRCBUF_NEXT(Id) \
   { \
      Id##_Idx++; \
      if( Id##_Idx > maxIdx_##Id ) \
         Id##_Idx = 0; \
   }


#endif

